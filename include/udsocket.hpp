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
#include <active_object.hpp>

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

    using DelayedResultCallback = std::function<void(const send_result_t result, const types::socket_handle_t& recipient, const std::span<const std::byte> data)>;
    using ReceiveCallback = std::function<void(ud_socket_t& self, const types::socket_handle_t& sender, const std::span<const std::byte> data)>;


    ud_socket_t(
        utils::active_object_ptr_t active_object,
        const types::socket_handle_t& own_handle,
        std::optional<ReceiveCallback> on_receive
    );

    ud_socket_t(utils::active_object_ptr_t active_object) noexcept;

    ~ud_socket_t() noexcept;

    send_result_t send(
        const types::socket_handle_t& recipient,
        std::span<const std::byte> data,
        std::optional<DelayedResultCallback> on_delayed_result
    ) noexcept;

    struct pending_message_t {
        types::socket_handle_t recipient;
        std::vector<std::byte> data;
        size_t bytes_already_written; // For future SOCK_STREAM support
        DelayedResultCallback on_delayed_result;
    };

private:

    void init() noexcept;

    void on_alive() override;

    void on_dead() override;

    void drain_received_messages();

    void drain_retriable_messages();

    utils::active_object_ptr_t m_active_object;

    int m_socketfd;

    const std::optional<const types::socket_handle_t> m_own_handle;
    const std::optional<ReceiveCallback> m_on_receive;

    std::mutex m_mutex;

    std::queue<pending_message_t> m_pending_messages;
    std::unordered_map<types::socket_handle_t, sockaddr_un> m_cache;

    std::shared_ptr<spdlog::logger> m_logger;
};



} // namespace threesomeip::ipc


#endif // _UDSOCKET_HPP