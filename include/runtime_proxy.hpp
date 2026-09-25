#ifndef _RUNTIME_PROXY_HPP
#define _RUNTIME_PROXY_HPP

/*=====*\
 * C++ *
\*=====*/
#include <cstdint>
#include <string>
#include <span>
#include <filesystem>
#include <optional>
#include <cstddef>
#include <functional>
#include <vector>

/*=============*\
 * APPLICATION *
\*=============*/
#include <configuration.hpp>
#include <configurable.hpp>
#include <udsocket.hpp>
#include <active_object.hpp>
#include <timer.hpp>
#include <awaitable.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/
#include <spdlog/spdlog.h>


namespace fs = std::filesystem;


namespace threesomeip::runtime {
using namespace threesomeip;


class runtime_proxy_t: public configurable_t {
public:
    using MessageReceivedCallback = std::function<void(std::span<const std::byte>)>;

    runtime_proxy_t(
        fs::path configuration_path,
        utils::active_object_ptr_t active_object,
        std::string_view app_name,
        uint16_t app_id,
        std::span<const config::service_configuration_t> offered_services,
        std::span<const config::service_configuration_t> requested_services
    ) noexcept;

    ipc::send_result_t send(std::span<const std::byte> someip_payload, std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb);

    uint16_t get_id() const {
        return m_app_id;
    }

    void register_message_listener(MessageReceivedCallback);

private:


    utils::awaitable_t<ipc::send_result_t> register_application();
    utils::awaitable_t<ipc::send_result_t> unregister_application();
    utils::awaitable_t<ipc::send_result_t> offer_services();
    utils::awaitable_t<ipc::send_result_t> request_services();

    std::vector<std::byte> wrap_with_ipc_header(const std::span<const std::byte> data, ipc::types::message_type_t message_type) const;


    utils::detached_task_t reconnect(std::function<void(bool)> on_done);


    void send_heartbeat();

    void handle_on_receive(
        ipc::ud_socket_t& self,
        const ipc::types::socket_handle_t& sender,
        const std::span<const std::byte> data
    ) noexcept;


    utils::active_object_ptr_t m_active_object;
    const std::string m_app_name;
    const uint16_t m_app_id;

    ipc::types::socket_handle_t m_own_socket_handle;
    ipc::types::socket_handle_t m_runtime_handle;
    std::vector<config::service_configuration_t> m_offered_services;
    std::vector<config::service_configuration_t> m_requested_services;

    std::vector<MessageReceivedCallback> m_registered_listeners;

    ipc::ud_socket_t m_socket;
    bool m_runtime_online;
    bool m_reconnect_in_progress;
    std::shared_ptr<utils::timer_handle_t> m_heartbeat;

    std::shared_ptr<spdlog::logger> m_logger;
};

} // namespace threesomeip::runtime

#endif // _RUNTIME_PROXY_HPP