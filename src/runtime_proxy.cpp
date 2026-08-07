/*=====*\
 * C++ *
\*=====*/
#include <string>
#include <cstdint>
#include <format>
#include <filesystem>
#include <array>
#include <cstddef>
#include <cstring>

/*=============*\
 * APPLICATION *
\*=============*/
#include <runtime_proxy.hpp>
#include <serialization.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/


namespace fs = std::filesystem;


namespace threesomeip::runtime {

runtime_proxy_t::runtime_proxy_t(const fs::path& sockets_path, std::string_view app_name, uint16_t app_id, std::string_view runtime_name) noexcept:
    m_own_socket_handle((sockets_path / std::format("{}_{}.sock", app_name, app_id)).filename()),
    m_runtime_handle((sockets_path / std::format("{}.sock", runtime_name)).filename()),
    m_socket(m_own_socket_handle, std::bind_front(&runtime_proxy_t::handle_on_receive, this)) {

}

bool runtime_proxy_t::registerApplication(const std::string_view app_name, const uint16_t app_id) {

    
    std::array<std::byte, threesomeip::ipc::MAX_PAYLOAD_SIZE> message_buffer{};
    threesomeip::ipc::serdes::serialize(message_buffer.data(), app_name, app_id);


}





} // namespace threesomeip
