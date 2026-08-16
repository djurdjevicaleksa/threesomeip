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
    m_app_name(app_name),
    m_app_id(app_id),
    m_own_socket_handle((sockets_path / std::format("{}_{}.sock", m_app_name, m_app_id)).string()),
    m_runtime_handle((sockets_path / std::format("{}.sock", runtime_name)).string()),
    m_socket(m_own_socket_handle, std::bind_front(&runtime_proxy_t::handle_on_receive, this)) {

}


bool runtime_proxy_t::registerApplication() {
    using namespace threesomeip::ipc;

    std::array<std::byte, MAX_PAYLOAD_SIZE> message_buffer{};

    /* calculate the size of the serialized header */
    const size_t header_length{serdes::serialize_dry_run(threesomeip::ipc::ipc_message_header_t{})};

    /* serialize the payload */
    size_t payload_length = serdes::serialize(message_buffer.data() + header_length, m_app_name, m_app_id);

    /* construct the header with the correct payload size and serialize it */
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
        ._flags{static_cast<uint8_t>(0)},
        ._request_id{static_cast<uint16_t>(0)},
        ._reserved{static_cast<uint16_t>(0)},
        .payload_length{static_cast<uint16_t>(payload_length)},
    };
    serdes::serialize(message_buffer.data(), message_header);

    return send_result_t::SENT == m_socket.send(m_runtime_handle, std::span{message_buffer}.subspan(0, header_length + payload_length), std::nullopt);
}

bool runtime_proxy_t::unregisterApplication() {
    using namespace threesomeip::ipc;

    std::array<std::byte, MAX_PAYLOAD_SIZE> message_buffer{};

    /* calculate the size of the serialized header */
    const size_t header_length{serdes::serialize_dry_run(threesomeip::ipc::ipc_message_header_t{})};

    /* serialize the payload */
    size_t payload_length = serdes::serialize(message_buffer.data() + header_length, m_app_name, m_app_id);

    /* construct the header with the correct payload size and serialize it */
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
        .message_type{threesomeip::ipc::message_type_t::UNREGISTER_APPLICATION},
        ._flags{static_cast<uint8_t>(0)},
        ._request_id{static_cast<uint16_t>(0)},
        ._reserved{static_cast<uint16_t>(0)},
        .payload_length{static_cast<uint16_t>(payload_length)},
    };
    serdes::serialize(message_buffer.data(), message_header);

    return send_result_t::SENT == m_socket.send(m_runtime_handle, std::span{message_buffer}.subspan(0, header_length + payload_length), std::nullopt);
}

bool runtime_proxy_t::offerServices(std::span<config::service_configuration_t> services) {
    using namespace threesomeip::ipc;

    std::array<std::byte, MAX_PAYLOAD_SIZE> message_buffer{};

    /* calculate the size of the serialized header */
    const size_t header_length{serdes::serialize_dry_run(threesomeip::ipc::ipc_message_header_t{})};

    /* serialize the payload */
    size_t payload_length = serdes::serialize(message_buffer.data() + header_length, services);

    /* construct the header with the correct payload size and serialize it */
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
        .message_type{threesomeip::ipc::message_type_t::OFFER_SERVICE},
        ._flags{static_cast<uint8_t>(0)},
        ._request_id{static_cast<uint16_t>(0)},
        ._reserved{static_cast<uint16_t>(0)},
        .payload_length{static_cast<uint16_t>(payload_length)},
    };
    serdes::serialize(message_buffer.data(), message_header);

    return send_result_t::SENT == m_socket.send(m_runtime_handle, std::span{message_buffer}.subspan(0, header_length + payload_length), std::nullopt);
}


bool runtime_proxy_t::requestServices(std::span<config::service_configuration_t> services) {}


} // namespace threesomeip
