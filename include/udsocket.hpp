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
#include <memory>

/*=============*\
 * APPLICATION *
\*=============*/
#include <lifecycle_listener.hpp>
#include <spdlog/logger.h>


namespace threesomeip::ipc {


enum class send_result_t {
    SENT = 0,
    DELAYED_RESULT,
    SOCKET_DEAD,
    RECIPIENT_AWAY
};

class ud_socket_t: public lifecycle_listener_t {
public:


    using DelayedResultCallback = std::function<void(const std::string_view recipient, const send_result_t result, const std::span<const std::byte> data)>;
    using ReceiveCallback = std::function<void(ud_socket_t& self, const std::string_view sender_pathname, const std::span<const std::byte> data)>;

    struct pending_message_t {
        const std::string recipient;
        const std::vector<std::byte> data;
        size_t bytes_already_written; // For future SOCK_STREAM support
        DelayedResultCallback on_delayed_result;
    };


    ud_socket_t(
        std::string_view own_pathname,
        std::optional<ReceiveCallback> on_receive
    ) noexcept;

    ud_socket_t() noexcept;

    ~ud_socket_t() noexcept;

    send_result_t send(
        std::string_view recepient_pathname,
        std::span<const std::byte> data,
        std::optional<DelayedResultCallback> on_delayed_result
    ) noexcept;

private:

    void init() noexcept;

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

    std::shared_ptr<spdlog::logger> m_logger;
};



} // namespace threesomeip::ipc


#endif // _UDSOCKET_HPP