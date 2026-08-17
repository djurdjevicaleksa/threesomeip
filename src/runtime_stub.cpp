/*=====*\
 * C++ *
\*=====*/
#include <string>
#include <format>
#include <functional>
#include <iostream>
#include <tuple>

/*=============*\
 * APPLICATION *
\*=============*/
#include <runtime_stub.hpp>
#include <ipc_format.hpp>
#include <serialization.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/
#include <spdlog/sinks/stdout_color_sinks.h>


namespace fs = std::filesystem;

namespace threesomeip::runtime {
using namespace threesomeip::ipc;

runtime_stub_t::runtime_stub_t(const fs::path& sockets_path, std::string_view runtime_application_name) noexcept:
    m_own_socket_handle((sockets_path / std::format("{}.sock", runtime_application_name)).string()),
    m_socket(m_own_socket_handle, std::bind_front(&runtime_stub_t::handle_on_receive, this)),
    m_logger(spdlog::stdout_color_mt("RUNTIME", spdlog::color_mode::always)) {
        m_logger->set_level(spdlog::level::debug);
        m_logger->set_pattern("[%H:%M:%S.%e][%n][%l] %v");
    }

void runtime_stub_t::handle_on_receive(
    ud_socket_t& self,
    const socket_handle_t& sender,
    const std::span<const std::byte> data
) noexcept {

    m_logger->debug("Received {} bytes of data from {} .", data.size(), sender);

    std::byte* payload_cursor{nullptr};
    const auto header = serdes::deserialize<ipc_message_header_t>(data.data(), &payload_cursor);

    m_logger->debug("Start of frame: {}", std::string_view(reinterpret_cast<const char*>(header.start_of_frame.data()), header.start_of_frame.size()));
    m_logger->debug("Protocol version: {}", std::to_string(header.protocol_version));
    m_logger->debug("Message type: {}", ipc_message_type_name(header.message_type));
    m_logger->debug("Payload length: {}", std::to_string(header.payload_length));

    switch (header.message_type) {
        case message_type_t::REGISTER_APPLICATION: {
            const auto message = serdes::deserialize<ipc_register_message_t>(payload_cursor);
            m_logger->debug("Application registered: {} ({})", message.application_name, message.application_id);
            break;
        }

        case message_type_t::UNREGISTER_APPLICATION: {
            const auto message = serdes::deserialize<ipc_unregister_message_t>(payload_cursor);
            m_logger->debug("Application unregistered: {} ({})", message.application_name, message.application_id);
            break;
        }

        case message_type_t::OFFER_SERVICE: {
            const auto message = serdes::deserialize<ipc_offer_services_message_t>(payload_cursor);
            m_logger->debug("Application {} offers the following services:", sender);
            for (const auto service: message) {
                m_logger->debug("Service {}, instance {}", service.service_id, service.instance_id);
            }
            break;
        }

        case message_type_t::REQUEST_SERVICE: {
            const auto message = serdes::deserialize<ipc_request_services_message_t>(payload_cursor);
            m_logger->debug("Application {} requires the following services:", sender);
            for (const auto service: message) {
                m_logger->debug("Service {}, instance {}", service.service_id, service.instance_id);
            }
            break;
        }
    }
}

std::string_view runtime_stub_t::ipc_message_type_name(message_type_t type) {
    switch (type) {
        case message_type_t::REGISTER_APPLICATION: return "REGISTER_APPLICATION";
        case message_type_t::UNREGISTER_APPLICATION: return "UNREGISTER_APPLICATION";
        case message_type_t::OFFER_SERVICE: return "OFFER_SERVICE";
        case message_type_t::REQUEST_SERVICE: return "REQUEST_SERVICE";
        default: return "!UNKNOWN TYPE!";
    }
}

} // namespace threesomeip::runtime