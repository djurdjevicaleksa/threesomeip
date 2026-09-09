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
#include <spdlog/spdlog.h>


namespace fs = std::filesystem;


namespace threesomeip::runtime {
using namespace threesomeip;


runtime_stub_t::runtime_stub_t(fs::path configuration_path, utils::active_object_ptr_t active_object) noexcept:
    configurable_t(configuration_path),
    m_active_object(active_object),
    m_own_socket_handle((m_ecu_configuration.sockets_path / std::format("{}.sock", m_ecu_configuration.runtime_application_name)).string()),
    m_socket(m_active_object, m_own_socket_handle, std::bind_front(&runtime_stub_t::handle_on_receive, this)),
    m_reliable(m_active_object, m_ecu_configuration.unicast_address, 40000, std::bind_front(&runtime_stub_t::handle_on_receive_reliable, this)),
    m_eviction_timer(
        utils::timer_factory::make_periodic_timer(m_active_object, std::chrono::seconds(10), [this] () {
            auto current_time = std::chrono::steady_clock::now();

            /* apps are copied intentionally */
            auto apps_to_evict = m_heartbeat_by_recency
                | std::views::take_while(
                    [current_time] (const heartbeat_t& ref) {
                        return (current_time - ref.timepoint) >= std::chrono::seconds(10);
                    }
                )
                | std::ranges::to<std::vector>();

            assert(apps_to_evict.size() > 0);

            for (const auto& app: apps_to_evict) {
                m_logger->debug("Evicting {} due to inactivity", m_apps.at(app.sender).app_name);
                this->evict_application(m_apps.at(app.sender));
            }

            if (m_heartbeat_by_recency.size() > 0) {
                m_eviction_timer->reschedule(std::chrono::seconds(10) - (std::chrono::steady_clock::now() - m_heartbeat_by_recency.begin()->timepoint));
                m_eviction_timer->start();
            }
            else {
                m_eviction_timer->reschedule(std::chrono::seconds(10));
            }
        })
    )
{
    auto& sinks = spdlog::get(std::string{m_active_object->get_name()})->sinks();
    m_logger = std::make_shared<spdlog::logger>("RTStub", sinks.begin(), sinks.end());
#ifdef RUNTIME_COMM_DEBUG
    m_logger->set_level(spdlog::level::debug);
#else
    m_logger->set_level(spdlog::level::off);
#endif // RUNTIME_COMM_DEBUG
    spdlog::register_logger(m_logger);
}

void runtime_stub_t::handle_on_receive(
    ipc::ud_socket_t& self,
    const ipc::types::socket_handle_t& sender,
    const std::span<const std::byte> data
) noexcept {
    (void) self;

    std::byte* payload_cursor{nullptr};
    const auto header = someip::serdes::deserialize<ipc::types::message_header_t>(data.data(), &payload_cursor);

    /* TODO add header check */

    switch (header.message_type) {
        case ipc::types::message_type_t::REGISTER_APPLICATION: {
            /* all registered apps must immediately apply for heartbeat checking because it can happen
            that an app is registered but it never sent a heartbeat; effectively skipping the eviction check */

            const auto message = someip::serdes::deserialize<ipc::types::register_message_t>(payload_cursor);
            m_apps.insert(sender, application_entry_t{sender, message.app_id, std::move(message.app_name), {}, {} });

            auto it = m_heartbeat_by_recency.emplace(m_heartbeat_by_recency.end(), sender, std::chrono::steady_clock::now());
            m_heartbeat_lookup.emplace(sender, it);

            m_logger->info("{} registered with ID {}", m_apps.at(sender).app_name, m_apps.at(sender).app_id);

            if (!m_eviction_timer->is_running()) {
                m_eviction_timer->start();
            }

            break;
        }

        case ipc::types::message_type_t::UNREGISTER_APPLICATION: {
            const auto message = someip::serdes::deserialize<ipc::types::unregister_message_t>(payload_cursor);
            m_logger->info("{} unregistered with ID {}", m_apps.at(sender).app_name, m_apps.at(sender).app_id);

            if (m_heartbeat_by_recency.size() == 1) {
                m_eviction_timer->reschedule(std::chrono::seconds(10));
            }

            this->evict_application(m_apps.at(sender));
            break;
        }

        case ipc::types::message_type_t::OFFER_SERVICE: {
            const auto message = someip::serdes::deserialize<ipc::types::offer_message_t>(payload_cursor);

            if (message.size()) {
                m_logger->info("{} offers the following services:", m_apps.at(sender).app_name);
            }
            else {
                m_logger->info("{} does not offer any services", m_apps.at(sender).app_name);
                break;
            }

            m_apps.at(sender).offered_services = std::move(message);
            for (const auto& service: m_apps.at(sender).offered_services) {
                m_apps.alias(sender, service.service_id);
                m_logger->info("Service {}, instance {}", service.service_id, service.instance_id);
            };

            break;
        }

        case ipc::types::message_type_t::REQUEST_SERVICE: {
            const auto message = someip::serdes::deserialize<ipc::types::request_message_t>(payload_cursor);

            if (message.size()) {
                m_logger->info("{} requests the following services:", m_apps.at(sender).app_name);
            }
            else {
                m_logger->info("{} does not request any services", m_apps.at(sender).app_name);
                break;
            }

            m_apps.at(sender).requested_services = std::move(message);
            for (const auto& service: m_apps.at(sender).requested_services) {
                m_logger->info("Service {}, instance {}", service.service_id, service.instance_id);
            }

            break;
        }

        case ipc::types::message_type_t::SEND: {
            m_logger->debug("Received a SOME/IP payload meant for another application.");

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

            ipc::types::socket_handle_t* recipient{nullptr};
            switch (someip_message_header.message_type) {
                case someip::types::message_type_t::REQUEST: [[fallthrough]];
                case someip::types::message_type_t::REQUEST_NO_RETURN: {
                    auto service_provider = m_apps.find(someip_message_header.message_id.service_id);
                    if (nullptr == service_provider) {
                        /* no such service; send reply */
                        m_logger->warn("Failed to send SOME/IP payload: no recipient with service {}", someip_message_header.message_id.service_id);
                        return;
                    }
                    recipient = &(service_provider->handle);
                    break;
                }
                case someip::types::message_type_t::RESPONSE: {
                    const auto key = makeRequestKey(someip_message_header);
                    recipient = &(m_apps.find(key)->handle);
                    m_apps.remove_alias(key);
                    break;
                }
                default: {
                    m_logger->warn("Failed to send SOME/IP payload: invalid SOME/IP message type");
                    return;
                }
            }

            if (someip_message_header.message_type == someip::types::message_type_t::REQUEST) {
                m_apps.alias(sender, makeRequestKey(someip_message_header));
            }

            m_socket.send(*recipient, std::span{message_buffer}.subspan(0, ipc_header_length + entire_someip_message.size()), std::nullopt);
            m_logger->debug("Forwarded a SOME/IP payload to {}", m_apps.at(*recipient).app_name);

            break;
        }

        case ipc::types::message_type_t::HEARTBEAT: {
            /* application's first contact with the runtime is a successful heartbeat; runtime must skip this if thats the case */
            if (!m_heartbeat_lookup.contains(sender)) break;

            bool most_stale_heartbeat_changed = false;

            /* if heartbeat received the list must have at least 1 element */
            /* remove previous heartbeat */
            if (m_heartbeat_lookup.at(sender) == m_heartbeat_by_recency.begin()) most_stale_heartbeat_changed = true;
            m_heartbeat_by_recency.erase(m_heartbeat_lookup.at(sender));

            /* add the new one */
            auto it = m_heartbeat_by_recency.emplace(m_heartbeat_by_recency.end(), sender, std::chrono::steady_clock::now());
            m_heartbeat_lookup.at(sender) = it;

            if (most_stale_heartbeat_changed) {
                /* reschedule timer */
                m_eviction_timer->reschedule(std::chrono::seconds(10) - (std::chrono::steady_clock::now() - m_heartbeat_by_recency.begin()->timepoint));
                m_eviction_timer->start();
            }

            break;
        }
    }
}

/* pure someip; no ipc header */
void runtime_stub_t::handle_on_receive_reliable(const std::string& address, const int port, const std::span<const std::byte> data) noexcept {
    std::byte* cursor{nullptr};
    const auto someip_header = someip::serdes::deserialize<someip::types::message_header_t>(data.data(), &cursor);
    (void) someip_header;

    // ....
}

/* socket_handle must not be passed from an internal data structure; its captured by reference */
void runtime_stub_t::evict_application(const application_entry_t& app) {
    /* remove from heartbeat cache */
    m_heartbeat_by_recency.erase(m_heartbeat_lookup.at(app.handle));
    m_heartbeat_lookup.erase(app.handle);
    m_apps.erase(app.handle);
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