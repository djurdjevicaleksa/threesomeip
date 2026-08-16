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

/*=============*\
 * APPLICATION *
\*=============*/
#include <configuration.hpp>
#include <udsocket.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/


namespace fs = std::filesystem;


namespace threesomeip::runtime {
using namespace threesomeip;

class runtime_proxy_t {
public:
    runtime_proxy_t(
        const fs::path& sockets_path,
        std::string_view app_name,
        uint16_t app_id,
        std::string_view runtime_name,
        std::span<const config::service_configuration_t> offered_services,
        std::span<const config::service_configuration_t> requested_services
    ) noexcept;


    ipc::send_result_t registerApplication(std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb);
    ipc::send_result_t unregisterApplication(std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb);
    ipc::send_result_t offerServices(std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb);
    ipc::send_result_t requestServices(std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb);

private:

    // void handle_delayed_socket_response(
    //     const threesomeip::ipc::send_result_t result,
    //     const threesomeip::ipc::socket_handle_t& recipient,
    //     const std::span<const std::byte> data
    // ) noexcept;

    void handle_on_receive(
        [[maybe_unused]] threesomeip::ipc::ud_socket_t& self,
        [[maybe_unused]] const threesomeip::ipc::socket_handle_t& sender,
        [[maybe_unused]] const std::span<const std::byte> data
    ) noexcept {};


    const std::string m_app_name;
    const uint16_t m_app_id;

    threesomeip::ipc::socket_handle_t m_own_socket_handle;
    threesomeip::ipc::socket_handle_t m_runtime_handle;
    std::vector<threesomeip::config::service_configuration_t> m_offered_services;
    std::vector<threesomeip::config::service_configuration_t> m_requested_services;

    threesomeip::ipc::ud_socket_t m_socket;
};

} // namespace threesomeip::runtime

#endif // _RUNTIME_PROXY_HPP