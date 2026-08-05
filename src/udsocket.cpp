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


/*
    TODO
    MOVED POLLFD[] TO A MEMBER VARIABLE - IMPLEMENT ITS SETTING AND IT UNSETTING IN SERVE()
*/


namespace threesomeip::ipc {


ud_socket_t::ud_socket_t(std::string_view own_pathname, std::optional<ReceiveCallback> on_receive) noexcept :
    m_ud_socket_fd(-1), m_wakeup_fd(-1), m_fdpoll_mask(POLLIN), m_pathname(own_pathname), m_on_receive(std::move(on_receive.value_or(nullptr))), m_writeonly(false) {

    if (this->init())   this->to_alive();
    else                this->to_dead();
}

ud_socket_t::ud_socket_t() noexcept :
    m_ud_socket_fd(-1), m_wakeup_fd(-1), m_fdpoll_mask(0), m_writeonly(true) {

    if (this->init())   this->to_alive();
    else                this->to_dead();
}

ud_socket_t::~ud_socket_t() {
    if (this->is_alive()) this->to_dead();

    /* wake the thread */
    uint64_t flag{1};
    write(m_wakeup_fd, &flag, sizeof(flag));

    if(t_worker.joinable()) {
        t_worker.join();
    }

    if (-1 != m_ud_socket_fd) {
        close(m_ud_socket_fd);
        m_ud_socket_fd = -1;
    }
    if (!m_writeonly) {
        unlink(m_pathname.value().c_str());
    }
}

bool ud_socket_t::init() {
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
        if (m_writeonly) return true;

        /* Bind to a filesystem file */
        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        std::snprintf(
            address.sun_path,
            sizeof(address.sun_path),
            "%s",
            m_pathname.value().c_str()
        );

        if (-1 == unlink(address.sun_path)) {
            break;
        }

        if (-1 == bind(m_ud_socket_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address))) {
            break;
        }

        /* Set up the thread wakeup event */
        if (int wakeup = eventfd(0, EFD_CLOEXEC); -1 == wakeup) {
            break;
        } else m_wakeup_fd = wakeup;

        /* Start the thread */
        t_worker = std::thread(&ud_socket_t::serve, this);

        return true;

    } while(false);

    if (-1 != m_ud_socket_fd) {
        close(m_ud_socket_fd);
        m_ud_socket_fd = -1;
    }
    return false;
}


bool ud_socket_t::send(std::string_view recepient_pathname, std::span<const std::byte> data, std::optional<TrivialCallback> _EXPERIMENTAL_on_fatal_error) {
    if (!m_cache.contains(recepient_pathname)) {
        sockaddr_un& entry = m_cache[recepient_pathname];
        entry.sun_family = AF_UNIX;
        std::snprintf(
            entry.sun_path,
            sizeof(entry.sun_path),
            "%s",
            recepient_pathname.data()
        );
    }

    sockaddr_un& entry = m_cache.at(recepient_pathname);

_write:

    if (data.size() != sendto(m_ud_socket_fd, data.data(), data.size(), 0, reinterpret_cast<sockaddr*>(&entry), sizeof(entry))) {
        switch (errno) {
#if defined(EAGAIN) && defined(EWOULDBLOCK)
    #if EAGAIN == EWOULDBLOCK
            case EAGAIN:
    #else
            case EAGAIN: [[fallthrough]];
            case EWOULDBLOCK:
    #endif
#elif defined(EAGAIN)
            case EAGAIN:
#elif defined(EWOULDBLOCK)
            case EWOULDBLOCK:
#else
    #error "Missing both errno values for missing memory for IPC transport!"
#endif
            /* case EAGAIN || EWOULDBLOCK */ {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_pending_messages.emplace(
                    std::string{recepient_pathname},
                    std::vector{data},
                    /* bytes_already_written always 0 for UNIX DGRAM */ 0,
                    std::move(_EXPERIMENTAL_on_fatal_error.value_or(nullptr))
                );

                // Register for writeable notifications
                m_fdpoll_mask |= POLLOUT;

                return true;

            }

            case EINTR: /* interrupted; try again */goto _write;

            default: {
                if (_EXPERIMENTAL_on_fatal_error.has_value() && _EXPERIMENTAL_on_fatal_error.value())
                std::invoke(_EXPERIMENTAL_on_fatal_error.value());
                return false;
            }
        }
    }

    return true;
}

bool ud_socket_t::receive() {

    // Prepare the output buffer
    std::array<std::byte, MAX_PAYLOAD_SIZE> buffer;

    // Prepare the struct for storing the sender's address
    sockaddr_un sender_address{};
    socklen_t sender_address_length{sizeof(sender_address)};

    ssize_t bytes_read{0};

_read:

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
#if defined(EAGAIN) && defined(EWOULDBLOCK)
    #if EAGAIN == EWOULDBLOCK
            case EAGAIN:
    #else
            case EAGAIN: [[fallthrough]];
            case EWOULDBLOCK:
    #endif
#elif defined(EAGAIN)
            case EAGAIN:
#elif defined(EWOULDBLOCK)
            case EWOULDBLOCK:
#else
    #error "Missing both errno values for missing memory for IPC transport!"
#endif
            /* case EAGAIN || EWOULDBLOCK: */ /* nothing to read */ {
                return false;
            }

            case EINTR: /* interrupted; try again */ {
                goto _read;
            }

            case ECONNREFUSED: /* try again */ {
                goto _read;
            }

            default: {
                return false;
            }
        }
    }

    const auto sender_pathname = std::string_view{
        sender_address.sun_path,
        sender_address_length - offsetof(sockaddr_un, sun_path)
    };

    if (!m_cache.contains(sender_pathname)) {
        m_cache.emplace(sender_pathname, std::move(sender_address));
    }

    if (m_on_receive.has_value() and m_on_receive.value()) {
        std::invoke(
            m_on_receive.value(),
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
            /* Disable read notifications because the socket wont read */
            .events = m_writeonly ? POLLOUT : POLLIN | POLLOUT
        },
        {
            .fd = m_wakeup_fd,
            .events = POLLIN
        }
    };

    while (true) {
        if (poll(poll_fds, 2, -1) < 0) continue;

        /* shutdown wakeup; drain the buffer */
        if (poll_fds[1].revents & POLLIN) {
            uint64_t read_data{0};
            read(m_wakeup_fd, &read_data, sizeof(read_data));
            break;
        }

        /* read buffer ready */
        if (!m_writeonly && poll_fds[0].revents & POLLIN) {
            while (this->receive()) {}
        }

        /* retry sending */
        if (!m_pending_messages.empty() && poll_fds[0].revents & POLLOUT) {
            while (send(
                m_pending_messages.front().recipient.
            ))


            send()

        }

    }

}


} // namespace threesomeip::ipc