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

/*=============*\
 * APPLICATION *
\*=============*/
#include <udsocket.hpp>
#include <ipc_format.hpp>


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


namespace threesomeip::ipc {


ud_socket_t::ud_socket_t(std::string_view own_pathname, std::optional<ReceiveCallback> on_receive) noexcept :
    m_ud_socket_fd(-1), m_wakeup_fd(-1), m_pathname(own_pathname), m_on_receive(std::move(on_receive.value_or(nullptr))) {
    this->init();
}

ud_socket_t::ud_socket_t() noexcept :
    m_ud_socket_fd(-1), m_wakeup_fd(-1) {
    this->init();
}

void ud_socket_t::init() {
    do {
        if (const int sock = socket(AF_UNIX, SOCK_DGRAM, 0); -1 == sock) {
            /* currently no issue which can arise is recoverable */
            break;
        }
        else m_ud_socket_fd = sock;

        /* Set into non-blocking state */
        int flags = fcntl(m_ud_socket_fd, F_GETFL, 0);
        if (-1 == fcntl(m_ud_socket_fd, F_SETFL, flags | O_NONBLOCK)) {
            break;
        }

        /* If its not named, it's done */
        if (!m_pathname.has_value()) {
            this->to_alive();
            return;
        }

        /* Bind to a filesystem file */
        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        std::snprintf(
            address.sun_path,
            sizeof(address.sun_path),
            "%s",
            m_pathname.value().c_str()
        );

        (void) unlink(address.sun_path);

        if (-1 == bind(m_ud_socket_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address))) {
            break;
        }

        /* Set up eventfds */
        if (int wakeup = eventfd(0, EFD_CLOEXEC); -1 == wakeup) {
            break;
        } else m_wakeup_fd = wakeup;

        if (int shutdown = eventfd(0, EFD_CLOEXEC); -1 == shutdown) {
            break;
        } else m_shutdown_fd = shutdown;

        this->to_alive();
        return;

    } while(false);

    this->to_dead();
}

void ud_socket_t::on_alive() {
    t_worker = std::thread(&ud_socket_t::serve, this);
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

    if (m_pathname.has_value()) {
        unlink(m_pathname.value().c_str());
    }
}

ud_socket_t::~ud_socket_t() {
    this->to_dead();
}

bool ud_socket_t::send(std::string_view recepient_pathname, std::span<const std::byte> data, std::optional<TrivialCallback> _EXPERIMENTAL_on_fatal_error) {
    if (!this->is_alive()) return false;
    if (data.size() > MAX_PAYLOAD_SIZE) return false;

    /* protected against TOCTOU */

    std::lock_guard<std::mutex> lock(m_mutex);

    /* immediately cache the recipient's address */
    if (!m_cache.contains(recepient_pathname)) {
        sockaddr_un& entry = m_cache.emplace(std::string{recepient_pathname}, sockaddr_un{}).first->second;
        entry.sun_family = AF_UNIX;
        std::snprintf(
            entry.sun_path,
            sizeof(entry.sun_path),
            "%s",
            recepient_pathname.data()
        );
    }

    /* "goto" alternative for when EINTR happens; still a single send attempt */
    while (true) {
        if (static_cast<ssize_t>(data.size()) != sendto(
            m_ud_socket_fd,
            data.data(),
            data.size(),
            0,
            reinterpret_cast<sockaddr*>(&m_cache.find(recepient_pathname)->second),
            sizeof(sockaddr_un))
        ) {
            switch (errno) {
                CASE_EAGAIN_EWOULDBLOCK: {
                    m_pending_messages.emplace(
                        std::string{recepient_pathname},
                        std::vector(data.begin(), data.end()),
                        /* bytes_already_written always 0 for UNIX DGRAM */ 0,
                        std::move(_EXPERIMENTAL_on_fatal_error.value_or(nullptr))
                    );

                    /* Register for writeable notifications and notify */
                    uint64_t flag{1};
                    write(m_wakeup_fd, &flag, sizeof(flag));

                    return true;
                }

                case EINTR: /* interrupted; try again */ continue;

                default: {
                    this->to_dead();
                    return false;
                }
            }
        }
        else break;
    }

    return true;
}

bool ud_socket_t::retry_send() {
    if (!this->is_alive()) return false;

    /* protected against TOCTOU */

    /* "goto" alternative for when EINTR happens; still a single retry */
    while (true) {
        if (static_cast<ssize_t>(m_pending_messages.front().data.size()) != sendto(
            m_ud_socket_fd,
            m_pending_messages.front().data.data(),
            m_pending_messages.front().data.size(),
            0,
            reinterpret_cast<const sockaddr*>(&m_cache.find(m_pending_messages.front().recipient)->second),
            sizeof(sizeof(sockaddr_un))
        )) {
            switch (errno) {
                CASE_EAGAIN_EWOULDBLOCK: {
                    /* it became blocking again */
                    return false;
                }

                case EINTR: /* interrupted; try again */ continue;

                default: {
                    /* exceptional case; unsupported */
                    if (m_pending_messages.front()._EXPERIMENTAL_on_fatal_error) {
                        std::invoke(m_pending_messages.front()._EXPERIMENTAL_on_fatal_error);
                    }

                    this->to_dead();
                    return false;
                }
            }
        }
        else break;
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

                case EINTR: /* interrupted; try again */ {
                    continue;
                }

                case ECONNREFUSED: /* try again */ {
                    continue;
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

    std::unique_lock<std::mutex> lock(m_mutex);

    const auto sender_pathname = std::string_view{sender_address.sun_path};

    if (!m_cache.contains(sender_pathname)) {
        m_cache.emplace(std::string{sender_pathname}, sender_address);
    }

    if (m_on_receive.has_value() and m_on_receive.value()) {
        lock.unlock();

        std::invoke(
            m_on_receive.value(),
            *this,
            sender_pathname,
            std::span<std::byte>(buffer).subspan(0, bytes_read)
        );
    }

    return true;
}

void ud_socket_t::serve() {
    pollfd poll_fds[]{
        {
            .fd = m_ud_socket_fd,
            .events = m_pathname.has_value() ? short{POLLIN} : short{0},
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
            while(read(m_wakeup_fd, &flag, sizeof(flag)) > 0) {}

            /* subscribe to "writeable" notifications */
            poll_fds[0].events |= POLLOUT;
        }

        if (socketfd_writeable()) {
            std::lock_guard<std::mutex> lock(m_mutex);

            /* retry sending */
            while (this->retry_send()) {}

            /* unsubscribe from writeable if all are sent  */
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
            while(read(m_shutdown_fd, &read_data, sizeof(read_data)) > 0) {}

            /* break and exit */
            break;
        }
    }
}

} // namespace threesomeip::ipc