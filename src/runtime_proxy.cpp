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
#include <chrono>

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
    m_own_socket_handle((sockets_path / std::format("{}_{}.sock", app_name, app_id)).string()),
    m_runtime_handle((sockets_path / std::format("{}.sock", runtime_name)).string()),
    m_socket(m_own_socket_handle, std::bind_front(&runtime_proxy_t::handle_on_receive, this)) {}

void runtime_proxy_t::send(const threesomeip::ipc::socket_handle_t& recipient, std::span<const std::byte> data) {
    using namespace threesomeip::ipc;

    std::chrono::seconds runtimeDisconnectedTimeout{5};

    while (send_result_t::RECIPIENT_AWAY == m_socket.send(m_runtime_handle, data, std::nullopt)) {
        std::this_thread::sleep_for(runtimeDisconnectedTimeout);
    }
}

bool runtime_proxy_t::registerApplication(const std::string_view app_name, const uint16_t app_id) {
    using namespace threesomeip::ipc;
    std::array<std::byte, MAX_PAYLOAD_SIZE> message_buffer{};

    size_t payload_length = serdes::serialize(message_buffer.data() + SERIALIZED_MESSAGE_HEADER_SIZE, app_name, app_id);

    threesomeip::ipc::ipc_message_header_t message_header{
        .start_of_frame{
            static_cast<std::byte>('#'),
            static_cast<std::byte>('t'),
            static_cast<std::byte>('h'),
            static_cast<std::byte>('r'),
            static_cast<std::byte>('e'),
            static_cast<std::byte>('e'),
            static_cast<std::byte>('s'),
            static_cast<std::byte>('o'),
            static_cast<std::byte>('m'),
            static_cast<std::byte>('e'),
            static_cast<std::byte>('i'),
            static_cast<std::byte>('p'),
            static_cast<std::byte>('#'),
        },
        .protocol_version{1},
        .message_type{threesomeip::ipc::message_type_t::REGISTER_APPLICATION},
        .payload_length{payload_length},
    };

    size_t header_length = serdes::serialize(
        message_buffer.data(),
        message_header
    );

    this->send(m_runtime_handle, std::span{message_buffer}.subspan(0, header_length + payload_length));
    return true;
}

} // namespace threesomeip
