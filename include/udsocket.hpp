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
#include <comm_ipc.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/
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

    using DelayedResultCallback = std::function<void(const send_result_t result, const socket_handle_t& recipient, const std::span<const std::byte> data)>;
    using ReceiveCallback = std::function<void(ud_socket_t& self, const socket_handle_t& sender, const std::span<const std::byte> data)>;

    struct pending_message_t {
        socket_handle_t recipient;
        std::vector<std::byte> data;
        size_t bytes_already_written; // For future SOCK_STREAM support
        DelayedResultCallback on_delayed_result;
    };


    ud_socket_t(
        const socket_handle_t& self,
        std::optional<ReceiveCallback> on_receive
    ) noexcept;

    ud_socket_t() noexcept;

    ~ud_socket_t() noexcept;

    send_result_t send(
        const socket_handle_t& recipient,
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

    const std::optional<const socket_handle_t> m_self;
    const std::optional<ReceiveCallback> m_on_receive;

    std::thread t_worker;
    std::mutex m_mutex;

    std::queue<pending_message_t> m_pending_messages;
    std::unordered_map<socket_handle_t, sockaddr_un> m_cache;

    std::shared_ptr<spdlog::logger> m_logger;
};



} // namespace threesomeip::ipc


#endif // _UDSOCKET_HPP