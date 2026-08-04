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

/*=============*\
 * APPLICATION *
\*=============*/
#include <udsocket.hpp>
#include <ipc_format.hpp>


/*
    TODO
    - understand how epoll works and how the kernel will notify the event loop owner that fds are readable/writeable,
    then implement a method for the socket which will read/write from/to those fds
    - Check if this works with sockaddr (sockaddr_un) since; it mentioned fds but im storing sockaddrs in pending
    requests
    - Make pending requests per-fd/sockaddr ??
*/


namespace threesomeip::ipc {


ud_socket_t::ud_socket_t(): m_ud_socket_fd(-1), m_pathname(std::nullopt) {
    if (this->init())   this->to_alive();
    else                this->to_dead();
}

ud_socket_t::ud_socket_t(std::string_view pathname): m_ud_socket_fd(-1), m_pathname(std::string{pathname}) {
    if (this->init())   this->to_alive();
    else                this->to_dead();
}

ud_socket_t::~ud_socket_t() {
    if (-1 != m_ud_socket_fd) {
        close(m_ud_socket_fd);
        m_ud_socket_fd = -1;
    }
    if (m_pathname.has_value()) {
        unlink(m_pathname.value().c_str());
    }
}

bool ud_socket_t::init() {
    if (const int sock = socket(AF_UNIX, SOCK_DGRAM, 0); -1 == sock) {
        switch (errno) {
            // Same error; not enough memory - retry later
            case ENOBUFS: [[fallthrough]];
            case ENOMEM: {
                // TODO recover
                break;
            }
            default: {
                // Non recoverable issues; for now at least
                return false;
            }
        }
    }
    else m_ud_socket_fd = sock;

    // Set into non-blocking state
    int flags = fcntl(m_ud_socket_fd, F_GETFL, 0);
    if (-1 == fcntl(m_ud_socket_fd, F_SETFL, flags | O_NONBLOCK)) {
        // TODO Handle errors
        close(m_ud_socket_fd);
        m_ud_socket_fd = -1;
        return false;
    }

    // If its not named, it's done
    if (!m_pathname.has_value()) return true;

    // Bind to a filesystem file
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    std::snprintf(
        address.sun_path,
        sizeof(address.sun_path) - 1,
        "%s",
        m_pathname.value().c_str()
    );

    if (-1 == unlink(address.sun_path)) {
        // TODO Handle error
        close(m_ud_socket_fd);
        m_ud_socket_fd = -1;
        return false;
    }

    if (-1 == bind(m_ud_socket_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address))) {
        // TODO Handle error
        close(m_ud_socket_fd);
        m_ud_socket_fd = -1;
        return false;
    }

    return true;
}

int ud_socket_t::fd() const {
    return m_ud_socket_fd;
}

bool ud_socket_t::send_to(int fd, std::span<std::byte> data, std::function<void(send_result_t)> on_completion) {

}

bool ud_socket_t::send_to(std::string_view pathname, std::span<std::byte> data, std::function<void(send_result_t)> on_completion) {
    // Format the recipient address
    sockaddr_un recipient{};
    recipient.sun_family = AF_UNIX;
    std::snprintf(
        recipient.sun_path,
        sizeof(recipient.sun_path) - 1,
        "%s",
        pathname.data()
    );

    if (data.size() == sendto(m_ud_socket_fd, data.data(), data.size(), 0, reinterpret_cast<sockaddr*>(&recipient), sizeof(recipient))) {
        switch (errno) {
#if defined(EAGAIN) && defined(EWOULDBLOCK)
    #if EAGAIN == EWOULDBLOCK
            case EAGAIN: {
    #else
            case EAGAIN: [[fallthrough]];
            case EWOULDBLOCK: {
    #endif
#elif defined(EAGAIN)
            case EAGAIN: {
#elif defined(EWOULDBLOCK)
            case EWOULDBLOCK: {
#else
    #error "Missing both errno values for missing memory for IPC transport!"
#endif
                // Handle if it were to block
                m_pending_messages.emplace(
                    recipient,
                    std::vector{data},
                    /* bytes_already_written always 0 for UNIX DGRAM */ 0,
                    std::move(on_completion)
                );

            }
            default: {
                if (on_completion) {
                    on_completion(send_result_t::DECLINED);
                    return false;
                }
            }
        }
    }

    if (on_completion) {
        on_completion(send_result_t::ACCEPTED);
        return true;
    }
}





} // namespace threesomeip::ipc