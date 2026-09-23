
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <functional>
#include <array>
#include <cstddef>
#include <memory>

#include <tcpsocket.hpp>
#include <active_object.hpp>
#include <awaitable.hpp>

#include <spdlog/spdlog.h>


#if defined(EAGAIN) && defined(EWOULDBLOCK)
    #if EAGAIN == EWOULDBLOCK
        #define CASE_EAGAIN_EWOULDBLOCK     case EAGAIN
    #else
        #define CASE_EAGAIN_EWOULDBLOCK     case EAGAIN: [[fallthrough]]; case EWOULDBLOCK
    #endif
#elif defined(EAGAIN)
        #define CASE_EAGAIN_EWOULDBLOCK     case EAGAIN
#elif defined(EWOULDBLOCK)
        #define CASE_EAGAIN_EWOULDBLOCK     case EWOULDBLOCK
#else
    #error "Missing both errno values for indicating that accept() would block."
#endif


namespace threesomeip::net {


tcp_socket_t::tcp_socket_t(utils::active_object_ptr_t active_object, const std::string& address, const int port, ReceiveCallback on_receive):
    m_active_object(active_object), m_on_receive(std::move(on_receive)), m_fd(-1), m_address(address), m_port(port) {
    this->init();
}

void tcp_socket_t::init() noexcept {

    auto& sinks = spdlog::get(std::string{m_active_object->get_name()})->sinks();
    m_logger = std::make_shared<spdlog::logger>("reliable", sinks.begin(), sinks.end());
#ifdef SOCKET_DEBUG
    m_logger->set_level(spdlog::level::debug);
#else
    m_logger->set_level(spdlog::level::off);
#endif // SOCKET_DEBUG
    spdlog::register_logger(m_logger);

    do {
        if (const int sock = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, IPPROTO_TCP); -1 == sock) {
            /* currently no issue which can arise is recoverable */
            m_logger->error("Failed to open a socket");
            break;
        }
        else m_fd = sock;

        m_logger->debug("Opened a socket");

        sockaddr_in own_address{};

        own_address.sin_family = AF_INET;
        own_address.sin_port = htons(static_cast<short unsigned int>(m_port));
        inet_pton(AF_INET, m_address.c_str(), &own_address.sin_addr);

        if (-1 == bind(m_fd, reinterpret_cast<const sockaddr*>(&own_address), sizeof(own_address))) {
            /* currently no issue can arise which is recoverable */
            m_logger->error("Failed to bind");
            break;
        }

        if (-1 == listen(m_fd, 1000)) {
            /* currently no issue can arise which is recoverable */
            m_logger->error("Failed to listen");
            break;
        }

        /* register incoming connection handling */
        (void) m_active_object->add_fd_to_readable_watchlist(m_fd, std::bind_front(&tcp_socket_t::accept_connections, this));

        this->to_alive();

        return;

    } while (false);

    this->to_dead();
}

void tcp_socket_t::on_alive() {
    m_logger->debug("Socket proclaimed alive");
}

void tcp_socket_t::on_dead() {
    (void) m_active_object->remove_fd_from_readable_watchlist(m_fd);
    if (-1 != m_fd) {
        close(m_fd);
        m_fd = -1;
    }

    m_logger->debug("Socket proclaimed dead");
}

utils::detached_task_t tcp_socket_t::send_to(const std::string& address, const int port, std::span<const std::byte> data, std::optional<DelayedResultCallback> on_delayed_result) {
    auto owned_data = std::make_shared<std::vector<std::byte>>(data.begin(), data.end());

    int fd = co_await this->connect_or_reuse_connection(address, port);

    if (fd < 0) {
        if (on_delayed_result) [[likely]] {
            on_delayed_result.value()(send_result_t::EPHEMERAL_SOCKET_DEAD, address, port, *owned_data);
        }
        co_return;
    }

    send_result_t result = co_await this->write_all(fd, owned_data);
    if (on_delayed_result) [[likely]] {
        on_delayed_result.value()(result, address, port, *owned_data);
    }
}

utils::awaitable_t<int> tcp_socket_t::connect_or_reuse_connection(const std::string& address, const int port) {
    const endpoint_t endpoint{address, port};
    if (m_connection_fds.contains(endpoint)) {
        int fd = m_connection_fds.at(endpoint);
        return utils::awaitable_t<int>([fd] (std::function<void(int)> resume) {
            resume(fd);
        });
    }

    return utils::awaitable_t<int>([this, address, port] (std::function<void(int)> resume) {
        int result = this->_internal_detail_connect_to(address, port, [resume] (int fd) { resume(fd); });

        /* if we immediately know the outcome */
        if (-2 != result) {
            resume(result);
        }
    });
}

utils::awaitable_t<send_result_t> tcp_socket_t::write_all(const int fd, std::shared_ptr<std::vector<std::byte>> data) {
    return utils::awaitable_t<send_result_t>(
        [this, fd, data] (std::function<void(send_result_t)> resume) -> void {
            size_t bytes_written{0};
            while (bytes_written < data->size()) {
                ssize_t increment = ::send(fd, data->data() + bytes_written, data->size() - bytes_written, MSG_NOSIGNAL);

                if (-1 == increment) {
                    switch (errno) {
                        CASE_EAGAIN_EWOULDBLOCK: {
                            m_connection_details.at(fd).pending_messages.emplace(
                                *data,
                                bytes_written,
                                [resume] (send_result_t result_, const std::string&, int, std::span<const std::byte>) { resume(result_); }
                            );
                            (void) m_active_object->add_fd_to_writeable_watchlist(fd, std::bind_front(&tcp_socket_t::retry_messages, this, fd));
                            return;
                        }

                        case EINTR: {
                            /* interrupted; try again */
                            continue;
                        }

                        default: {
                            /* exceptional case; evict connection */
                            this->evict_connection(fd);
                            resume(send_result_t::EPHEMERAL_SOCKET_DEAD);
                            return;
                        }
                    }
                }
                else if (0 == increment) {
                    m_logger->warn("Unexpectedly sent 0 bytes");

                    m_connection_details.at(fd).pending_messages.emplace(
                        *data,
                        bytes_written,
                        [resume] (send_result_t result_, const std::string&, int, std::span<const std::byte>) { resume(result_); }
                    );
                    (void) m_active_object->add_fd_to_writeable_watchlist(fd, std::bind_front(&tcp_socket_t::retry_messages, this, fd));
                    return;
                }
                else {
                    bytes_written += increment;
                }
            }

            resume(send_result_t::SENT);
        }
    );
}

/*
    Creates and connects an ephemeral socket used for sending data. Cleans up after itself.
    Returns -1 on fatal error, -2 on delayed response or a valid fd on success.
*/
int tcp_socket_t::_internal_detail_connect_to(const std::string& address, const int port, std::function<void(int)> on_delayed_result) {
    const int sock = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, IPPROTO_TCP);

    if (-1 == sock) {
        /* currently no issue which can arise is recoverable */
        m_logger->error("Failed to open a socket for outgoing connection");
        return -1;
    }

    m_logger->debug("Opened a socket for outgoing connection");

    sockaddr_in own_address{
        .sin_family{AF_INET},
        .sin_port{htons(0)},
        .sin_addr{},
        .sin_zero{}
    };
    inet_pton(AF_INET, m_address.c_str(), &own_address.sin_addr);

    if (-1 == bind(sock, reinterpret_cast<const sockaddr*>(&own_address), sizeof(own_address))) {
        /* currently no issue can arise which is recoverable */
        m_logger->error("Failed to bind for outgoing connection");
        close(sock);
        return -1;
    }

    sockaddr_in recipient_address{
        .sin_family{AF_INET},
        .sin_port{htons(static_cast<uint16_t>(port))},
        .sin_addr{},
        .sin_zero{}
    };
    inet_pton(AF_INET, address.c_str(), &recipient_address.sin_addr);

    if (-1 == ::connect(sock, reinterpret_cast<const sockaddr*>(&recipient_address), sizeof(recipient_address))) {
        switch (errno) {
            /* wait for delayed notification of connection */
            case EINPROGRESS: {
                (void) m_active_object->add_fd_to_writeable_watchlist(sock, [this, sock, cb = std::move(on_delayed_result), address, port] () -> void {
                    (void) m_active_object->remove_fd_from_writeable_watchlist(sock);

                    int socket_error{0};
                    socklen_t length{sizeof(socket_error)};
                    getsockopt(sock, SOL_SOCKET, SO_ERROR, &socket_error, &length);

                    if (socket_error == 0) {

                        /* add it to the list of connections */
                        endpoint_t new_endpoint{address, port};
                        m_connection_fds.emplace(new_endpoint, sock);
                        m_connection_details.emplace(
                            sock,
                            connection_details_t{
                                std::move(new_endpoint),
                                {}
                            }
                        );
                        (void) m_active_object->add_fd_to_readable_watchlist(sock, std::bind_front(&tcp_socket_t::receive_messages, this, sock));

                        m_logger->debug("Connected for outgoing connection after a delay");
                        cb(sock);
                    }
                    else {
                        m_logger->error("Failed to connect for outgoing connection after a delay");
                        close(sock);
                        cb(-1);
                    }
                });

                return -2;
            }

            default: {
                /* exceptional case; unsupported */
                m_logger->error("Failed to connect for outgoing connection");
                close(sock);
                return -1;
            }
        }
    }

    /* add it to the list of connections */
    endpoint_t new_endpoint{address, port};
    m_connection_fds.emplace(new_endpoint, sock);
    m_connection_details.emplace(
        sock,
        connection_details_t{
            std::move(new_endpoint),
            {}
        }
    );
    (void) m_active_object->add_fd_to_readable_watchlist(sock, std::bind_front(&tcp_socket_t::receive_messages, this, sock));

    m_logger->debug("Connected for outgoing connection");
    return sock;
}


void tcp_socket_t::accept_connections() {
    /* this callback cannot be dispatched if the listening socket is dead; is_alive() check would cause confusion */

    /* returns true if it should continue accepting */
    const auto acceptSingleConnection = [this] () -> bool {
        sockaddr_in client_address{};
        socklen_t client_length{sizeof(client_address)};

        int client_fd{-1};

        /* "goto" alternative for when EINTR happens; still a single try */
        for (;;) {
            client_fd = accept(m_fd, reinterpret_cast<sockaddr*>(&client_address), &client_length);
            if (client_fd == -1) {
                switch(errno) {
                    CASE_EAGAIN_EWOULDBLOCK: return false; /* no connections to accept */

                    case ECONNABORTED: return true; /* connection aborted; continue */

                    case EINTR: continue; /* interrupted; try again */

                    default: {
                        /* exceptional case; unsupported */
                        this->to_dead();
                        return false;
                    }
                }
            }
            else break;
        }

        int flags = fcntl(client_fd, F_GETFL, 0);
        if (-1 == fcntl(client_fd, F_SETFL, flags | O_NONBLOCK)) {
            this->to_dead();
            return false;
        }

        std::array<char, INET_ADDRSTRLEN> client_native_address{};
        inet_ntop(AF_INET, &client_address.sin_addr, client_native_address.data(), client_native_address.size());
        int client_native_port{ntohs(client_address.sin_port)};
        endpoint_t client_endpoint{std::string{client_native_address.data(), client_native_address.size()}, client_native_port};

        m_connection_fds.emplace(client_endpoint, client_fd);
        m_connection_details.emplace(
            client_fd,
            connection_details_t{
                .recipient{std::move(client_endpoint)},
                .pending_messages{}
            }
        );

        /* register incoming message handling */
        (void) m_active_object->add_fd_to_readable_watchlist(client_fd, std::bind_front(&tcp_socket_t::receive_messages, this, client_fd));
        return true;
    };

    while (acceptSingleConnection()) {}
}

void tcp_socket_t::receive_messages(const int fd) {

    /* returns true if it should keep draining */
    const auto receiveSingleMessage = [this, fd] () -> bool {
        if (!this->is_alive()) return false;

        /* prepare output buffer */
        std::array<std::byte, /* TODO reevaluate the size */ 1400> message_buffer{};
        ssize_t bytes_read{0};

        /* "goto" alternative for when EINTR happens; still a single try */
        for (;;) {
            bytes_read = recv(fd, message_buffer.data(), message_buffer.size(), 0);

            /* peer done sending; graceful termination */
            if (0 == bytes_read) {
                /* TODO send data if needed */
                this->evict_connection(fd);

                return false;
            }
            else if (-1 == bytes_read) {
                switch (errno) {
                    CASE_EAGAIN_EWOULDBLOCK: /* nothing to read */ return false;

                    case EINTR: {
                        /* interrupted; try again */
                        continue;
                    }

                    /* connection reset by peer; disgraceful termination */
                    case ECONNRESET: {
                        this->evict_connection(fd);
                        return false;
                    }

                    default: {
                        /* exceptional case; unsupported */
                        this->to_dead();
                        return false;
                    }
                }
            }
            else break;
        }

        if (m_on_receive) [[likely]] {
            auto& entry = m_connection_details.at(fd);
            m_on_receive(entry.recipient.address, entry.recipient.port, std::span{message_buffer}.subspan(0, bytes_read));
        }
        return true;
    };

    while (receiveSingleMessage()) {}
}

void tcp_socket_t::retry_messages(const int fd) {

    /* returns true if it should keep draining */
    const auto retrySingleMessage = [this, fd] () -> bool {
        auto& connection_details = m_connection_details.at(fd);
        auto& pending_message = connection_details.pending_messages.front();

        size_t bytes_written{pending_message.bytes_already_written};
        while (bytes_written < pending_message.data.size()) {
            ssize_t increment = ::send(fd, pending_message.data.data() + bytes_written, pending_message.data.size() - bytes_written, MSG_NOSIGNAL);

            if (-1 == increment) {
                switch (errno) {
                    CASE_EAGAIN_EWOULDBLOCK: {
                        pending_message.bytes_already_written = bytes_written;
                        return false;
                    }

                    case EINTR: {
                        /* interrupted; try again */
                        continue;
                    }

                    default: {
                        /* exceptional case; evict connection */

                        if (pending_message.on_delayed_result) [[likely]] {
                            pending_message.on_delayed_result(
                                send_result_t::EPHEMERAL_SOCKET_DEAD,
                                connection_details.recipient.address,
                                connection_details.recipient.port,
                                pending_message.data
                            );
                        }

                        this->evict_connection(fd);
                        return false;
                    }
                }
            }
            else if (0 == increment) {
                m_logger->warn("Unexpectedly sent 0 bytes while retrying");
                pending_message.bytes_already_written = bytes_written;
                return false;
            }

            bytes_written += increment;
        }

        if (pending_message.on_delayed_result) [[likely]] {
            pending_message.on_delayed_result(
                send_result_t::SENT,
                connection_details.recipient.address,
                connection_details.recipient.port,
                pending_message.data
            );
        }

        connection_details.pending_messages.pop();
        if (0 == connection_details.pending_messages.size()) return false;
        else return true;
    };

    while (retrySingleMessage()) {}

    if (m_connection_details.contains(fd) && (0 == m_connection_details.at(fd).pending_messages.size())) {
        (void) m_active_object->remove_fd_from_writeable_watchlist(fd);

    }
}

void tcp_socket_t::evict_connection(const int fd) {
    if (-1 != fd) {
        (void) m_active_object->remove_fd_from_readable_watchlist(fd);
        (void) m_active_object->remove_fd_from_writeable_watchlist(fd);

        if (m_connection_details.contains(fd)) {
            m_connection_fds.erase(m_connection_details.at(fd).recipient);
            m_connection_details.erase(fd);
        }
        close(fd);
    }
}

} // namespace threesomeip::net
