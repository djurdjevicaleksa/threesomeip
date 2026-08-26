/*=====*\
 * C++ *
\*=====*/
#include <string>
#include <format>
#include <functional>
#include <tuple>
#include <ranges>

/*=============*\
 * APPLICATION *
\*=============*/
#include <runtime_stub.hpp>
#include <comm_ipc.hpp>
#include <serdes/serialization.hpp>
#include <configuration.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/
#include <spdlog/sinks/stdout_color_sinks.h>


namespace fs = std::filesystem;

namespace threesomeip::runtime {
using namespace threesomeip;

runtime_stub_t::runtime_stub_t(utils::active_object_ptr_t active_object, const fs::path& sockets_path, std::string_view runtime_application_name) noexcept:
    m_active_object(active_object),
    m_own_socket_handle((sockets_path / std::format("{}.sock", runtime_application_name)).string()),
    m_socket(m_active_object, m_own_socket_handle, std::bind_front(&runtime_stub_t::handle_on_receive, this)),
    m_logger(spdlog::stdout_color_mt("RUNTIME", spdlog::color_mode::always)) {
        m_logger->set_level(spdlog::level::debug);
        m_logger->set_pattern("[%H:%M:%S.%e][%n][%l] %v");
    }

void runtime_stub_t::handle_on_receive(
    ipc::ud_socket_t& self,
    const ipc::types::socket_handle_t& sender,
    const std::span<const std::byte> data
) noexcept {
    (void) self;

    m_logger->debug("Received {} bytes of data from {} .", data.size(), sender);

    std::byte* payload_cursor{nullptr};
    const auto header = someip::serdes::deserialize<ipc::types::message_header_t>(data.data(), &payload_cursor);

    m_logger->debug("Start of frame: {}", std::string_view{header.start_of_frame});
    m_logger->debug("Protocol version: {}", std::to_string(header.protocol_version));
    m_logger->debug("Message type: {}", message_type_name(header.message_type));
    m_logger->debug("Payload length: {}", std::to_string(header.payload_length));

    switch (header.message_type) {
        case ipc::types::message_type_t::REGISTER_APPLICATION: {
            const auto message = someip::serdes::deserialize<ipc::types::register_message_t>(payload_cursor);
            m_logger->debug("Application registered: {} ({})", message.app_name, message.app_id);
            m_applications.emplace(sender, application_entry_t{message.app_id, message.app_name});
            break;
        }

        case ipc::types::message_type_t::UNREGISTER_APPLICATION: {
            const auto message = someip::serdes::deserialize<ipc::types::unregister_message_t>(payload_cursor);
            m_logger->debug("Application unregistered: {} ({})", message.app_name, message.app_id);

            for (const auto& service: m_applications[sender].offered_services) {
                m_service_to_owner.erase(service.service_id);
            }

            m_applications.erase(sender);
            break;
        }

        case ipc::types::message_type_t::OFFER_SERVICE: {
            const auto message = someip::serdes::deserialize<ipc::types::offer_message_t>(payload_cursor);
            m_logger->debug("Application {} offers the following services:", sender);
            for (const auto service: message) {
                m_logger->debug("Service {}, instance {}", service.service_id, service.instance_id);
            }

            m_applications[sender].offered_services = message;

            std::ranges::for_each(message, [&] (const auto& service) {
                m_service_to_owner.emplace(service.service_id, sender);
            });
            break;
        }

        case ipc::types::message_type_t::REQUEST_SERVICE: {
            const auto message = someip::serdes::deserialize<ipc::types::request_message_t>(payload_cursor);
            m_logger->debug("Application {} requires the following services:", sender);
            for (const auto service: message) {
                m_logger->debug("Service {}, instance {}", service.service_id, service.instance_id);
            }
            m_applications[sender].requested_services = message;
            break;
        }

        case ipc::types::message_type_t::SEND: {
            m_logger->debug("Received SOME/IP payload to send.");

            std::span<const std::byte> entire_someip_message{payload_cursor, header.payload_length};
            const auto someip_message_header = someip::serdes::deserialize<someip::types::message_header_t>(payload_cursor);

            std::array<std::byte, ipc::MAX_PAYLOAD_SIZE> message_buffer{};

            /* construct the header and serialize it */
            ipc::types::message_header_t message_header{
                .start_of_frame{'#', 't', 'h', 'r', 'e', 'e', 's', 'o', 'm', 'e', 'i', 'p', '#'},
                .protocol_version{1},
                .message_type{ipc::types::message_type_t::SEND},
                ._flags{someip::types::uint8{0}},
                ._request_id{someip::types::uint16{0}},
                ._reserved{someip::types::uint16{0}},
                .payload_length{static_cast<someip::types::uint16>(entire_someip_message.size())},
            };
            size_t ipc_header_length = someip::serdes::serialize(message_buffer.data(), message_header);

            /* copy the already serialized payload containing the someip header and someip payload into the buffer */
            std::ranges::copy(entire_someip_message, message_buffer.begin() + ipc_header_length);


            const auto makeRequestKey = [] (const someip::types::message_header_t& _header) -> request_key_t {
                return request_key_t{
                    .service_id{_header.message_id.service_id},
                    .method_id{_header.message_id.method_id},
                    .client_id{_header.request_id.client_id},
                    .session_id{_header.request_id.session_id},
                };
            };

            ipc::types::socket_handle_t recipient{};
            switch (someip_message_header.message_type) {
                case someip::types::message_type_t::REQUEST: [[fallthrough]];
                case someip::types::message_type_t::REQUEST_NO_RETURN: {
                    auto it = m_service_to_owner.find(someip_message_header.message_id.service_id);
                    if (it == m_service_to_owner.end()) {
                        /* no such service; send reply */
                        m_logger->warn("Failed to send SOME/IP payload: no recipient with service {}", someip_message_header.message_id.service_id);
                        return;
                    }
                    recipient = it->second;
                    break;
                }
                case someip::types::message_type_t::RESPONSE: {
                    const auto key = makeRequestKey(someip_message_header);
                    recipient = m_pending_requests[key];
                    m_pending_requests.erase(key);
                    break;
                }
                default: {
                    m_logger->warn("Failed to send SOME/IP payload: invalid SOME/IP message type");
                    return;
                }
            }

            if (someip_message_header.message_type == someip::types::message_type_t::REQUEST) {
                m_pending_requests.emplace(makeRequestKey(someip_message_header), sender);
            }

            m_socket.send(recipient, std::span{message_buffer}.subspan(0, ipc_header_length + entire_someip_message.size()), std::nullopt);
            m_logger->debug("Successfully sent the SOME/IP payload to {}", recipient);

            break;
        }
    }
}

std::string_view runtime_stub_t::message_type_name(ipc::types::message_type_t type) const {
    switch (type) {
        case ipc::types::message_type_t::REGISTER_APPLICATION: return "REGISTER_APPLICATION";
        case ipc::types::message_type_t::UNREGISTER_APPLICATION: return "UNREGISTER_APPLICATION";
        case ipc::types::message_type_t::OFFER_SERVICE: return "OFFER_SERVICE";
        case ipc::types::message_type_t::REQUEST_SERVICE: return "REQUEST_SERVICE";
        case ipc::types::message_type_t::SEND: return "SEND";
        default: return "!UNKNOWN TYPE!";
    }
}

} // namespace threesomeip::runtime