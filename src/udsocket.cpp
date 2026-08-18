/*=====*\
 * C++ *
\*=====*/
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <sys/fcntl.h>
#include <array>
#include <vector>
#include <thread>
#include <mutex>
#include <optional>
#include <poll.h>
#include <sys/eventfd.h>
#include <filesystem>
#include <format>
#include <cstdlib>

/*=============*\
 * APPLICATION *
\*=============*/
#include <udsocket.hpp>
#include <comm_ipc.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/
#include <spdlog/sinks/stdout_color_sinks.h>


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
    #error "Missing both errno values for indicating that the socket's write buffer is full."
#endif

#define CASE_ENOENT_ECONNREFUSED case ENOENT: [[fallthrough]]; case ECONNREFUSED



namespace threesomeip::ipc {


ud_socket_t::ud_socket_t(const socket_handle_t& self, std::optional<ReceiveCallback> on_receive) noexcept:
    m_ud_socket_fd(-1), m_wakeup_fd(-1), m_self(self), m_on_receive(std::move(on_receive.value_or(nullptr))) {
    this->init();
}

ud_socket_t::ud_socket_t() noexcept:
    m_ud_socket_fd(-1), m_wakeup_fd(-1) {
    this->init();
}

void ud_socket_t::init() noexcept {

    if (m_self.has_value()) m_logger = spdlog::stdout_color_mt(std::filesystem::path(m_self.value()).filename(), spdlog::color_mode::always);
    else m_logger = spdlog::stdout_color_mt(std::format("unnamed_socket_{}", std::rand() % 1024));

    m_logger->set_level(spdlog::level::debug);
    m_logger->set_pattern("[%H:%M:%S.%e][%n][%l] %v");

    do {
        if (const int sock = socket(AF_UNIX, SOCK_DGRAM, 0); -1 == sock) {
            /* currently no issue which can arise is recoverable */
            m_logger->info("Failed to open a socket");
            break;
        }
        else m_ud_socket_fd = sock;
        m_logger->info("Opened a socket");

        /* Set into non-blocking state */
        int flags = fcntl(m_ud_socket_fd, F_GETFL, 0);
        if (-1 == fcntl(m_ud_socket_fd, F_SETFL, flags | O_NONBLOCK)) {
            m_logger->info("Failed to set to non-blocking mode");
            break;
        }
        m_logger->info("Set to non-blocking mode");

        /* If its not named, it's done */
        if (!m_self.has_value()) {
            this->to_alive();
            m_logger->info("Initialized");
            return;
        }

        /* Bind to a filesystem file */
        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        std::snprintf(
            address.sun_path,
            sizeof(address.sun_path),
            "%s",
            m_self.value().c_str()
        );

        (void) unlink(address.sun_path);

        if (-1 == bind(m_ud_socket_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address))) {
            m_logger->info("Failed to bind");
            break;
        }
        m_logger->info("Bound");

        /* Set up eventfds */
        if (int wakeup = eventfd(0, EFD_CLOEXEC); -1 == wakeup) {
            m_logger->info("Failed to initialize a wakeup eventfd");
            break;
        } else m_wakeup_fd = wakeup;
        m_logger->info("Initialized a wakeup eventfd");

        if (int shutdown = eventfd(0, EFD_CLOEXEC); -1 == shutdown) {
            m_logger->info("Failed to initialize a shutdown eventfd");
            break;
        } else m_shutdown_fd = shutdown;
        m_logger->info("Initialized a shutdown eventfd");

        this->to_alive();
        return;

    } while(false);

    this->to_dead();
}

void ud_socket_t::on_alive() {
    t_worker = std::thread(&ud_socket_t::serve, this);
    m_logger->info("Started a worker thread; Socket proclaimed alive");
}

void ud_socket_t::on_dead() {
    /* wake the thread */
    uint64_t flag{1};
    write(m_shutdown_fd, &flag, sizeof(flag));

    if(t_worker.joinable()) {
        t_worker.join();
    }

    if (-1 != m_ud_socket_fd) {
        close(m_ud_socket_fd);
        m_ud_socket_fd = -1;
    }

    if (m_self.has_value()) {
        unlink(m_self.value().c_str());
    }
    m_logger->info("Cleanup; Socket proclaimed dead");
}

ud_socket_t::~ud_socket_t() noexcept {
    this->to_dead();
}

auto ud_socket_t::send(
    const socket_handle_t& recipient,
    std::span<const std::byte> data,
    std::optional<DelayedResultCallback> on_delayed_result) noexcept
-> send_result_t {

    if (!this->is_alive()) return send_result_t::SOCKET_DEAD;

    /* protected against TOCTOU */
    std::lock_guard<std::mutex> lock(m_mutex);

    /* immediately cache the recipient's address */
    if (!m_cache.contains(recipient)) {
        sockaddr_un& entry = m_cache[recipient];
        entry.sun_family = AF_UNIX;
        std::snprintf(
            entry.sun_path,
            sizeof(entry.sun_path),
            "%s",
            recipient.data()
        );
    }

    /* "goto" alternative for when EINTR happens; still a single send attempt */
    while (true) {
        if (static_cast<ssize_t>(data.size()) != sendto(
            m_ud_socket_fd,
            data.data(),
            data.size(),
            0,
            reinterpret_cast<sockaddr*>(&m_cache[recipient]),
            sizeof(sockaddr_un))
        ) {
            switch (errno) {
                CASE_EAGAIN_EWOULDBLOCK: {
                    m_logger->debug("Socket kernel buffer full, will retry to send the message later");

                    m_pending_messages.emplace(
                        recipient,
                        std::vector(data.begin(), data.end()),
                        /* bytes_already_written always 0 for UNIX DGRAM */ 0,
                        std::move(on_delayed_result.value_or(nullptr))
                    );

                    /* Register for writeable notifications and notify */
                    uint64_t flag{1};
                    write(m_wakeup_fd, &flag, sizeof(flag));

                    return send_result_t::DELAYED_RESULT;
                }

                CASE_ENOENT_ECONNREFUSED: {
                    m_logger->debug(std::format("The recipient ({}) is unreachable", recipient));
                    return send_result_t::RECIPIENT_AWAY;
                }

                case EINTR: {
                    m_logger->debug("Send interrupted, will retry to send the message");
                    /* interrupted; try again */ continue;
                }

                default: {
                    m_logger->debug("Fatal error occurred, socket died");
                    this->to_dead();
                    return send_result_t::SOCKET_DEAD;
                }
            }
        }
        else break;
    }

    m_logger->debug(
        std::format(
            "Sent {} bytes of data to {} [{}]",
            data.size(),
            recipient,
            std::string_view(reinterpret_cast<const char*>(data.data()), data.size())
        )
    );
    return send_result_t::SENT;
}

bool ud_socket_t::retry_send() {
    if (!this->is_alive()) return false;

    /* protected against TOCTOU */
    std::unique_lock<std::mutex> lock(m_mutex);

    /* "goto" alternative for when EINTR happens; still a single retry */
    while (true) {
        if (static_cast<ssize_t>(m_pending_messages.front().data.size()) != sendto(
            m_ud_socket_fd,
            m_pending_messages.front().data.data(),
            m_pending_messages.front().data.size(),
            0,
            reinterpret_cast<const sockaddr*>(&m_cache[m_pending_messages.front().recipient]),
            sizeof(sockaddr_un)
        )) {
            switch (errno) {
                CASE_EAGAIN_EWOULDBLOCK: {
                    /* it became blocking again */
                    m_logger->debug("Socket kernel buffer full while trying to retry, will retry again later");
                    return false;
                }

                case EINTR: {
                    m_logger->debug("Retry interrupted, will retry again");
                    /* interrupted; try again */ continue;
                }

                CASE_ENOENT_ECONNREFUSED: {
                    m_logger->debug(std::format("The recipient ({}) is unreachable during retrying", m_pending_messages.front().recipient));

                    if (m_pending_messages.front().on_delayed_result) {
                        const auto pending_message = std::move(m_pending_messages.front());

                        lock.unlock();
                        std::invoke(
                            pending_message.on_delayed_result,
                            send_result_t::RECIPIENT_AWAY,
                            pending_message.recipient,
                            std::span<const std::byte>{pending_message.data}
                        );
                        lock.lock();
                    }
                    m_pending_messages.pop();
                    return true;
                }

                default: {
                    /* exceptional case; unsupported */
                    m_logger->debug("Fatal error occurred while retrying, socket died");

                    if (m_pending_messages.front().on_delayed_result) {
                        const auto pending_message = std::move(m_pending_messages.front());

                        lock.unlock();
                        std::invoke(
                            pending_message.on_delayed_result,
                            send_result_t::SOCKET_DEAD,
                            pending_message.recipient,
                            std::span<const std::byte>{pending_message.data}
                        );
                    }
                    this->to_dead();
                    return false;
                }
            }
        }
        else break;
    }

    m_logger->debug(
        std::format(
            "Sent {} bytes of data to {} during the retry sequence [{}]",
            m_pending_messages.front().data.size(),
            m_pending_messages.front().recipient,
            std::string_view(reinterpret_cast<const char*>(m_pending_messages.front().data.data()), m_pending_messages.front().data.size())
        )
    );
    if (m_pending_messages.front().on_delayed_result) {
        const auto pending_message = std::move(m_pending_messages.front());

        lock.unlock();
        std::invoke(
            pending_message.on_delayed_result,
            send_result_t::SENT,
            pending_message.recipient,
            std::span<const std::byte>{pending_message.data}
        );
        lock.lock();
    }
    m_pending_messages.pop();
    return true;
}

bool ud_socket_t::receive() {
    if (!this->is_alive()) return false;

    /* protected against TOCTOU */

    // Prepare the output buffer
    std::array<std::byte, MAX_PAYLOAD_SIZE> buffer;

    // Prepare the struct for storing the sender's address
    sockaddr_un sender_address{};
    socklen_t sender_address_length{sizeof(sender_address)};

    ssize_t bytes_read{0};

    while (true) {
        bytes_read = recvfrom(
            m_ud_socket_fd,
            buffer.data(),
            buffer.size(),
            0,
            reinterpret_cast<sockaddr*>(&sender_address),
            &sender_address_length
        );

        if (-1 == bytes_read) {
            switch (errno) {
                CASE_EAGAIN_EWOULDBLOCK: {
                    return false;
                }

                /* interrupted or bounceback */
                case EINTR: [[fallthrough]];
                case ECONNREFUSED: {
                    m_logger->debug("Read interrupted, will retry again");
                    continue;
                }

                default: {
                    /* exceptional case; unsupported */
                    m_logger->debug("Fatal error occurred while trying to read the buffer, socket died");
                    this->to_dead();
                    return false;
                }
            }
        }
        else break;
    }


    const auto sender_handle = socket_handle_t{sender_address.sun_path};

    m_logger->debug(
        std::format(
            "Received {} bytes of data [{}]",
            bytes_read,
            std::string_view(reinterpret_cast<const char*>(buffer.data()), bytes_read)
        )
    );


    std::unique_lock<std::mutex> lock(m_mutex);

    if (!m_cache.contains(sender_handle)) {
        m_cache.emplace(sender_handle, sender_address);
    }

    if (m_on_receive.has_value() and m_on_receive.value()) {
        lock.unlock();

        std::invoke(
            m_on_receive.value(),
            *this,
            sender_handle,
            std::span<std::byte>(buffer).subspan(0, bytes_read)
        );
    }

    return true;
}

void ud_socket_t::serve() {
    pollfd poll_fds[]{
        {
            .fd = m_ud_socket_fd,
            .events = m_self.has_value() ? short{POLLIN} : short{0},
            .revents{}
        },
        {
            .fd = m_wakeup_fd,
            .events = POLLIN, /* incoming */
            .revents{}
        },
        {
            .fd = m_shutdown_fd,
            .events = POLLIN, /* incoming */
            .revents{}
        }
    };

    const auto wakeupfd_readable = [&] { return poll_fds[1].revents & POLLIN; };
    const auto shutdownfd_readable = [&] { return poll_fds[2].revents & POLLIN; };
    const auto socketfd_writeable = [&] { return poll_fds[0].revents & POLLOUT; };
    const auto socketfd_readable = [&] { return poll_fds[0].revents & POLLIN; };


    while (true) {
        if (poll(poll_fds, sizeof(poll_fds) / sizeof(poll_fds[0]), -1) < 0) continue;

        if (wakeupfd_readable()) {
            /* drain */
            uint64_t flag{0};
            read(m_wakeup_fd, &flag, sizeof(flag));

            /* subscribe to "writeable" notifications */
            poll_fds[0].events |= POLLOUT;

            /* allow it to read the event immeditely; not in next iteration */
            poll_fds[0].revents |= POLLOUT;
        }

        if (socketfd_writeable()) {
            /* retry sending */
            while (this->retry_send()) {}

            /* unsubscribe from writeable if all are sent  */
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_pending_messages.empty()) {
                poll_fds[0].events &= ~POLLOUT;
            }
        }

        if (socketfd_readable()) {
            /* read messages */
            while(this->receive()) {}
        }

        if (shutdownfd_readable()) {
            /* drain the buffer */
            uint64_t read_data{0};
            read(m_shutdown_fd, &read_data, sizeof(read_data));

            /* break and exit */
            break;
        }
    }
}

} // namespace threesomeip::ipc