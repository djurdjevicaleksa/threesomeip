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
#include <thread>
#include <mutex>

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

    bool send_to(std::string_view pathname, std::span<const std::byte> data);

    bool recv_from(/* out */ std::vector<std::byte>& payload, /* out */ std::string& sender_pathname);

    // bool retry_send_to()

private:

    bool init();

    void send();

    int m_ud_socket_fd;
    const std::optional<std::string> m_pathname;
    std::thread t_worker;
    std::mutex m_mutex;

    std::queue<pending_message_t> m_pending_messages;
};



} // namespace threesomeip::ipc


#endif // _UDSOCKET_HPP