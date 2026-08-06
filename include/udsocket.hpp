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
    using TrivialCallback = std::function<void()>;


struct pending_message_t {
    const std::string recipient;
    const std::vector<std::byte> data;
    size_t bytes_already_written; // For future SOCK_STREAM support
    TrivialCallback _EXPERIMENTAL_on_fatal_error;
};


class ud_socket_t: public lifecycle_listener_t {
public:

    using ReceiveCallback = std::function<void(ud_socket_t& self, const std::string_view sender_pathname, const std::span<const std::byte> data)>;

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



    void init();

    bool receive();

    void serve();

    bool retry_send();

    void on_alive() override;

    void on_dead() override;


    int m_ud_socket_fd;

    int m_wakeup_fd; // Used for reapplying the fdpoll_mask
    int m_shutdown_fd;

    const std::optional<const std::string> m_pathname;
    const std::optional<ReceiveCallback> m_on_receive;

    std::thread t_worker;
    std::mutex m_mutex;

    std::queue<pending_message_t> m_pending_messages;

    /* make unordered_map<stding> indexable using string_view */
    struct string_hash {
        using is_transparent = void;
        size_t operator()(std::string_view sv) const noexcept {
            return std::hash<std::string_view>{}(sv);
        }
    };
    std::unordered_map<std::string, sockaddr_un, string_hash, std::equal_to<>> m_cache;
};



} // namespace threesomeip::ipc


#endif // _UDSOCKET_HPP