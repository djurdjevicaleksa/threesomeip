#ifndef _UDSOCKET_HPP
#define _UDSOCKET_HPP

/*=====*\
 * C++ *
\*=====*/
#include <vector>
#include <cstddef>
#include <string_view>
#include <functional>
#include <span>
#include <optional>
#include <queue>
#include <sys/un.h>

/*=============*\
 * APPLICATION *
\*=============*/
#include <lifecycle_listener.hpp>


namespace threesomeip::ipc {



enum class send_result_t {
    ACCEPTED = 0,
    DECLINED
};

struct pending_message_t {
    sockaddr_un recipient;
    const std::vector<const std::byte> data;
    size_t bytes_already_written; // For future SOCK_STREAM support
    std::function<void()> on_completion;
};


/*
    Currently only blocking
*/
class ud_socket_t: protected lifecycle_listener {
public:

    ud_socket_t();

    ud_socket_t(std::string_view pathname);

    ~ud_socket_t();

    int fd() const;

    bool send_to(int fd, std::span<std::byte> data, std::function<void(send_result_t)> on_completion);
    bool send_to(std::string_view pathname, std::span<std::byte> data, std::function<void(send_result_t)> on_completion);

    // bool retry_send_to()

private:

    bool init();

    void send();

    int m_ud_socket_fd;
    const std::optional<std::string> m_pathname;

    std::queue<pending_message_t> m_pending_messages;
};



} // namespace threesomeip::ipc


#endif // _UDSOCKET_HPP