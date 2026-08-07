#ifndef _RUNTIME_PROXY_HPP
#define _RUNTIME_PROXY_HPP

/*=====*\
 * C++ *
\*=====*/
#include <cstdint>
#include <string>
#include <span>
#include <filesystem>

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


class runtime_proxy_t {
public:

    runtime_proxy_t(const fs::path& sockets_path, std::string_view app_name, uint16_t app_id, std::string_view runtime_name) noexcept;
    ~runtime_proxy_t();

    bool registerApplication(const std::string_view app_name, const uint16_t app_id);
    bool unregisterApplication();
    bool offerServices(std::span<config::service_configuration_t> services);
    bool requestServices(std::span<config::service_configuration_t> services);

private:

    void handle_delayed_socket_response(
        const threesomeip::ipc::send_result_t result,
        const threesomeip::ipc::socket_handle_t& recipient,
        const std::span<const std::byte> data
    ) noexcept;

    void handle_on_receive(
        threesomeip::ipc::ud_socket_t& self,
        const threesomeip::ipc::socket_handle_t& sender,
        const std::span<const std::byte> data
    ) noexcept;


    std::string m_app_name;
    uint16_t m_app_id;

    std::string m_own_socket_handle;
    std::string m_runtime_handle;

    threesomeip::ipc::ud_socket_t m_socket;
};

} // namespace threesomeip::runtime

#endif // _RUNTIME_PROXY_HPP