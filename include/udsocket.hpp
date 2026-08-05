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
#include <unordered_map>

/*=============*\
 * APPLICATION *
\*=============*/
#include <lifecycle_listener.hpp>


namespace threesomeip::ipc {


struct pending_message_t {
    const std::string recipient;
    const std::vector<const std::byte> data;
    size_t bytes_already_written; // For future SOCK_STREAM support
    std::function<void()> _EXPERIMENTAL_on_fatal_error;
};


/*
    Currently only blocking
*/
class ud_socket_t: protected lifecycle_listener_t {
public:

    using ReceiveCallback = std::function<void(const std::string_view sender_pathname, const std::span<const std::byte> data)>;
    using TrivialCallback = std::function<void()>;

    ud_socket_t(
        std::string_view own_pathname,
        std::optional<ReceiveCallback> on_receive
    ) noexcept;

    ud_socket_t() noexcept;

    ~ud_socket_t();

    bool send(
        std::string_view recepient_pathname,
        std::span<const std::byte> data,
        std::optional<TrivialCallback> _EXPERIMENTAL_on_fatal_error
    );

private:

    bool init();

    bool receive();

    void serve();

    virtual void on_alive() const {}

    virtual void on_dead() const {}


    int m_ud_socket_fd;

    int m_wakeup_fd;
    short int m_fdpoll_mask;
    pollfd m_poll_fds[2];

    const std::optional<const std::string> m_pathname;
    std::optional<ReceiveCallback> m_on_receive;

    const bool m_writeonly;


    std::thread t_worker;
    std::mutex m_mutex;

    std::queue<pending_message_t> m_pending_messages;
    std::unordered_map<std::string_view, sockaddr_un> m_cache;
};



} // namespace threesomeip::ipc


#endif // _UDSOCKET_HPP