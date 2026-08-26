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
#include <active_object.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>


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


ud_socket_t::ud_socket_t(utils::active_object_ptr_t active_object, const types::socket_handle_t& own_handle, std::optional<ReceiveCallback> on_receive):
    m_active_object(active_object), m_socketfd(-1), m_own_handle(own_handle), m_on_receive(std::move(on_receive.value_or(nullptr))) {
    this->init();
}

ud_socket_t::ud_socket_t(utils::active_object_ptr_t active_object) noexcept:
    m_active_object(active_object), m_socketfd(-1) {
    this->init();
}

void ud_socket_t::init() noexcept {

    auto sinks = spdlog::get(std::string{m_active_object->get_name()})->sinks();
    m_logger = std::make_shared<spdlog::logger>("SOCK", sinks.begin(), sinks.end());

#ifdef SOCKET_DEBUG
    m_logger->set_level(spdlog::level::debug);
#else
    m_logger->set_level(spdlog::level::off);
#endif // SOCKET_DEBUG

    spdlog::register_logger(m_logger);

    do {
        if (const int sock = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0); -1 == sock) {
            /* currently no issue which can arise is recoverable */
            m_logger->info("Failed to open a socket");
            break;
        }
        else m_socketfd = sock;

        m_logger->info("Opened a socket");

        /* Set into non-blocking state */
        int flags = fcntl(m_socketfd, F_GETFL, 0);
        if (-1 == fcntl(m_socketfd, F_SETFL, flags | O_NONBLOCK)) {
            m_logger->info("Failed to set to non-blocking mode");
            break;
        }

        /* If its not named, it's done */
        if (!m_own_handle.has_value()) {
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
            m_own_handle.value().c_str()
        );

        (void) unlink(address.sun_path);

        if (-1 == bind(m_socketfd, reinterpret_cast<sockaddr*>(&address), sizeof(address))) {
            m_logger->info("Failed to bind");
            break;
        }
        m_logger->info("Bound");
        m_active_object->add_fd_to_readable_watchlist(m_socketfd, std::bind_front(&ud_socket_t::drain_received_messages, this));

        this->to_alive();
        return;

    } while(false);

    this->to_dead();
}

void ud_socket_t::on_alive() {
    m_logger->info("Socket proclaimed alive");
}

void ud_socket_t::on_dead() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (-1 != m_socketfd) {
            (void) m_active_object->remove_fd_from_readable_watchlist(m_socketfd);
            (void) m_active_object->remove_fd_from_writeable_watchlist(m_socketfd);
            (void) close(m_socketfd);
            m_socketfd = -1;
        }

        if (m_own_handle.has_value()) {
            unlink(m_own_handle.value().c_str());
        }
    }

    m_logger->info("Socket proclaimed dead");
}

ud_socket_t::~ud_socket_t() noexcept {
    this->to_dead();
}

auto ud_socket_t::send(
    const types::socket_handle_t& recipient,
    std::span<const std::byte> data,
    std::optional<DelayedResultCallback> on_delayed_result) noexcept
-> send_result_t {

    if (!this->is_alive()) return send_result_t::SOCKET_DEAD;

    std::unique_lock<std::mutex> lock(m_mutex);

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
    for(;;) {
        if (static_cast<ssize_t>(data.size()) != sendto(
            m_socketfd,
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

                    /* Register for writeable notifications */
                    m_active_object->add_fd_to_writeable_watchlist(m_socketfd, std::bind_front(&ud_socket_t::drain_retriable_messages, this));
                    return send_result_t::DELAYED_RESULT;
                }

                CASE_ENOENT_ECONNREFUSED: {
                    m_logger->debug(std::format("The recipient is unreachable"));
                    return send_result_t::RECIPIENT_AWAY;
                }

                case EINTR: {
                    m_logger->debug("Send interrupted, will retry to send the message");
                    /* interrupted; try again */ continue;
                }

                default: {
                    lock.unlock();
                    this->to_dead();
                    lock.lock();

                    m_logger->debug("Fatal error occurred, socket died");
                    return send_result_t::SOCKET_DEAD;
                }
            }
        }
        else break;
    }

    m_logger->debug("Sent {} bytes to {}:\n[{}]",
        data.size(),
        recipient,
        spdlog::to_hex(data.begin(), data.end())
    );
    return send_result_t::SENT;
}

void ud_socket_t::drain_retriable_messages() {

    /* returns true if it should keep draining */
    const auto retrySingleMessage = [this] () -> bool {
        if (!this->is_alive()) return false;

        std::unique_lock<std::mutex> lock(m_mutex);

        /* "goto" alternative for when EINTR happens; still a single retry */
        for (;;) {
            if (static_cast<ssize_t>(m_pending_messages.front().data.size()) != sendto(
                m_socketfd,
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
                            pending_message.on_delayed_result(
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
                            pending_message.on_delayed_result(
                                send_result_t::SOCKET_DEAD,
                                pending_message.recipient,
                                std::span<const std::byte>{pending_message.data}
                            );
                            lock.lock();
                        }

                        lock.unlock();
                        this->to_dead();
                        lock.lock();

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
            pending_message.on_delayed_result(
                send_result_t::SENT,
                pending_message.recipient,
                std::span<const std::byte>{pending_message.data}
            );
            lock.lock();
        }
        m_pending_messages.pop();
        return true;
    };

    /* drain retriables */
    while (retrySingleMessage()) {}

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_pending_messages.empty()) {
            (void) m_active_object->remove_fd_from_writeable_watchlist(m_socketfd);
        }
    }
}

void ud_socket_t::drain_received_messages() {

    /* returns true if it should keep draining */
    const auto receiveSingleMessage = [this] () -> bool {
        if (!this->is_alive()) return false;

        // Prepare the output buffer
        std::array<std::byte, MAX_PAYLOAD_SIZE> buffer;

        // Prepare the struct for storing the sender's address
        sockaddr_un sender_address{};
        socklen_t sender_address_length{sizeof(sender_address)};

        ssize_t bytes_read{0};

        for(;;) {
            bytes_read = recvfrom(
                m_socketfd,
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

        const auto sender_handle = types::socket_handle_t{sender_address.sun_path};

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

        if (m_on_receive.has_value()) [[likely]] {
            if (m_on_receive.value()) [[likely]] {
                lock.unlock();
                m_on_receive.value()(*this, sender_handle, std::span<std::byte>(buffer).subspan(0, bytes_read));
                lock.lock();
            }
        }

        return true;
    };

    /* drain received messages */
    while (receiveSingleMessage()) {}
}

} // namespace threesomeip::ipc