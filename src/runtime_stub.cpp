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


runtime_stub_t::runtime_stub_t(fs::path configuration_path, utils::active_object_ptr_t active_object):
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

    const auto ipc_header = someip::serdes::deserialize<ipc::types::message_header_t>(data.data());
    constexpr size_t ipc_header_size{someip::serdes::serialize_dry_run<ipc::types::message_header_t>()};
    const std::span<const std::byte> ipc_payload{data.subspan(ipc_header_size)};

    switch (ipc_header.message_type) {
        case ipc::types::message_type_t::REGISTER_APPLICATION: {
            this->handle_register_application(sender, ipc_payload);
            break;
        }
        case ipc::types::message_type_t::UNREGISTER_APPLICATION: {
            this->handle_unregister_application(sender, ipc_payload);
            break;
        }
        case ipc::types::message_type_t::OFFER_SERVICE: {
            this->handle_offer_services(sender, ipc_payload);
            break;
        }
        case ipc::types::message_type_t::REQUEST_SERVICE: {
            this->handle_request_services(sender, ipc_payload);
            break;
        }
        case ipc::types::message_type_t::SEND: {
            this->handle_send(sender, ipc_payload);
            break;
        }
        case ipc::types::message_type_t::HEARTBEAT: {
            this->handle_heartbeat(sender, ipc_payload);
            break;
        }
    }
}

void runtime_stub_t::handle_register_application(const ipc::types::socket_handle_t& sender, const std::span<const std::byte> data) {
    /* registered applications must immediately be registered for heartbeat tracking */

    auto message = someip::serdes::deserialize<ipc::types::register_message_t>(data.data());
    m_apps.insert(sender, application_entry_t{sender, message.app_id, std::move(message.app_name), {}, {} });

    auto it = m_heartbeat_by_recency.emplace(m_heartbeat_by_recency.end(), sender, std::chrono::steady_clock::now());
    m_heartbeat_lookup.emplace(sender, it);

    m_logger->info("{} registered with ID {}", m_apps.at(sender).app_name, m_apps.at(sender).app_id);

    if ( !m_eviction_timer->is_running()) {
        m_eviction_timer->start();
    }
}

void runtime_stub_t::handle_unregister_application(const ipc::types::socket_handle_t& sender, const std::span<const std::byte> data) {
    const auto message = someip::serdes::deserialize<ipc::types::unregister_message_t>(data.data());
    m_logger->info("{} unregistered with ID {}", m_apps.at(sender).app_name, m_apps.at(sender).app_id);

    if (m_heartbeat_by_recency.size() == 1) {
        m_eviction_timer->reschedule(std::chrono::seconds(10));
    }

    this->evict_application(m_apps.at(sender));
}

void runtime_stub_t::handle_offer_services(const ipc::types::socket_handle_t& sender, const std::span<const std::byte> data) {
    auto message = someip::serdes::deserialize<ipc::types::offer_message_t>(data.data());

    if (message.size()) {
        m_logger->info("{} offers the following services:", m_apps.at(sender).app_name);
    }
    else {
        m_logger->info("{} does not offer any services", m_apps.at(sender).app_name);
        return;
    }

    auto& owning_application = m_apps.at(sender);

    owning_application.offered_services = std::move(message);
    for (const auto& service: owning_application.offered_services) {
        m_apps.alias(sender, service.service_id);
        m_logger->info("Service {}, instance {}", service.service_id, service.instance_id);
    }
}

void runtime_stub_t::handle_request_services(const ipc::types::socket_handle_t& sender, const std::span<const std::byte> data) {
    auto message = someip::serdes::deserialize<ipc::types::request_message_t>(data.data());

    if (message.size()) {
        m_logger->info("{} requests the following services:", m_apps.at(sender).app_name);
    }
    else {
        m_logger->info("{} does not request any services", m_apps.at(sender).app_name);
        return;
    }

    auto& owning_application = m_apps.at(sender);

    owning_application.requested_services = std::move(message);
    for (const auto& service: owning_application.requested_services) {
        m_logger->info("Service {}, instance {}", service.service_id, service.instance_id);
    }
}

void runtime_stub_t::handle_send(const ipc::types::socket_handle_t& sender, const std::span<const std::byte> data) {
    m_logger->debug("Received a SOME/IP payload meant for another application.");

    if (data.size() > 1400) {
        m_logger->warn("{} tried to send a {}-byte SOME/IP payload, which exceeds the 1400-byte cap.", sender, data.size());
        return;
    }

    const auto someip_header = someip::serdes::deserialize<someip::types::message_header_t>(data.data());

    switch (someip_header.message_type) {
        /* request for a local or remote service */
        case threesomeip::someip::types::message_type_t::REQUEST: [[fallthrough]];
        case threesomeip::someip::types::message_type_t::REQUEST_NO_RETURN: {
            const auto requested_service_id = someip_header.message_id.service_id;

            /* check if requested service exists/is connected */
            if (
                !m_apps.contains(requested_service_id)
                && m_ecu_configuration.external_services.end() == std::ranges::find_if(m_ecu_configuration.external_services,
                    [requested_service_id] (const config::external_service_configuration_t& service) {
                        return requested_service_id == service.service_id;
                    }
                )
            ) {
                m_logger->warn("{} ({}) requested service {} which is unknown.",
                    m_apps.at(sender).app_name,
                    m_apps.at(sender).app_id,
                    requested_service_id
                );

                return;
            }

            if (m_apps.contains(requested_service_id)) {
                /* the service is local to this runtime */

                const auto complete_payload{this->wrap_with_ipc_header(data, ipc::types::message_type_t::SEND)};
                const auto& service_provider_handle = m_apps.at(requested_service_id).handle;

                if (someip_header.message_type == someip::types::message_type_t::REQUEST) {
                    m_pending_requests.emplace(this->makeRequestKey(someip_header), sender);
                }

                m_socket.send(service_provider_handle, complete_payload, std::nullopt);
                m_logger->debug("Forwarded a SOME/IP request to {}", m_apps.at(service_provider_handle).app_name);
            }
            else {
                /* already established that the external service exists at this point */
                const auto external_service = std::ranges::find_if(m_ecu_configuration.external_services,
                    [requested_service_id] (const config::external_service_configuration_t& service) {
                        return requested_service_id == service.service_id;
                    }
                );

                const auto& service_provider_address{external_service->ip};
                const auto service_provider_port{external_service->tcp_port};

                if (someip_header.message_type == someip::types::message_type_t::REQUEST) {
                    m_pending_requests.emplace(makeRequestKey(someip_header), sender);
                }

                m_reliable.send_to(service_provider_address, service_provider_port, data, std::nullopt);
                m_logger->debug("Forwarded a SOME/IP request to {}:{}", service_provider_address, service_provider_port);
            }
            break;
        }

        /* response for a local or remote client */
        case threesomeip::someip::types::message_type_t::RESPONSE: [[fallthrough]];
        case threesomeip::someip::types::message_type_t::ERROR: {
            const auto request_key{makeRequestKey(someip_header)};

            if ( !m_pending_requests.contains(request_key)) {
                m_logger->warn("Received a response without a registered request.");
                return;
            }

            auto& requester = m_pending_requests.at(request_key);

            if (std::holds_alternative<ipc::types::socket_handle_t>(requester)) {
                /* requester is local to this runtime */
                auto& requester_handle = std::get<ipc::types::socket_handle_t>(requester);

                if ( !m_apps.contains(requester_handle)) {
                    m_logger->warn("{} sent a response for {} which is no longer available.",
                        sender,
                        requester_handle
                    );

                    m_pending_requests.erase(request_key);
                    break;
                }

                const auto complete_payload{this->wrap_with_ipc_header(data, ipc::types::message_type_t::SEND)};

                m_socket.send(requester_handle, complete_payload, std::nullopt);
                m_logger->debug("Forwarded a SOME/IP response to {}", requester_handle);
                m_pending_requests.erase(request_key);
            }
            else {
                /* requester is remote */
                auto& requester_endpoint = std::get<net::endpoint_t>(requester);

                m_reliable.send_to(requester_endpoint.address, requester_endpoint.port, data, std::nullopt);
                m_logger->debug("Forwarded a SOME/IP response to {}:{}", requester_endpoint.address, requester_endpoint.port);

                m_pending_requests.erase(request_key);
            }

            break;
        }

        default: {
            m_logger->warn("Received an unsupported SOME/IP message type: {}", static_cast<uint8_t>(someip_header.message_type));
        }
    }
}

void runtime_stub_t::handle_heartbeat(const ipc::types::socket_handle_t& sender, const std::span<const std::byte> data) {
    if ( !m_heartbeat_lookup.contains(sender)) {
        m_logger->warn("Received a heartbeat from an unknown application: {}", sender);
        return;
    }

    bool most_stale_heartbeat_changed = false;

    /* if heartbeat was received the list must have at least 1 element */
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
}


void runtime_stub_t::handle_on_receive_reliable(const std::string& address, const int port, const std::span<const std::byte> data) noexcept {
    /* pure someip; no ipc header */
    /* create an assembly line if one doesn't exist already */
    auto& assembly_line = m_tcp_package_assembly[net::endpoint_t{address, port}];
    std::ranges::copy(data, std::back_inserter(assembly_line.data));

    /* see if the collected payload parts amount to an existing 'length' field */
    if (assembly_line.data.size() < size_t{8}) {
        return;
    }

    if (assembly_line.declared_payload_size == size_t{0}) {
        assembly_line.declared_payload_size = someip::serdes::deserialize<someip::types::uint32>(static_cast<std::byte*>(&assembly_line.data[4]));
    }

    if (assembly_line.data.size() < assembly_line.declared_payload_size /* message_id + length fields */ + size_t{8}) return;


    /* message assembled */
    m_logger->debug("Assembled a reliable package.");

    this->handle_on_reliable_assembled(address, port, std::span{assembly_line.data.data(), assembly_line.declared_payload_size + size_t{8}});

    assembly_line.data.erase(assembly_line.data.begin(), assembly_line.data.begin() + assembly_line.declared_payload_size + size_t{8});
    assembly_line.declared_payload_size = size_t{0};
}

void runtime_stub_t::handle_on_reliable_assembled(const std::string& address, const int port, const std::span<const std::byte> data) noexcept {
    if (data.size() > 1400) {
        m_logger->warn("Assembled a reliable {}-byte SOME/IP payload from {}:{}, which exceeds the 1400-byte cap.", data.size(), address, port);
        return;
    }

    const auto someip_header = someip::serdes::deserialize<someip::types::message_header_t>(data.data());

    switch (someip_header.message_type) {
        /* remote request for a local service */
        case threesomeip::someip::types::message_type_t::REQUEST: [[fallthrough]];
        case threesomeip::someip::types::message_type_t::REQUEST_NO_RETURN: {
            const auto requested_service = someip_header.message_id.service_id;

            if ( !m_apps.contains(requested_service)) {
                m_logger->warn("{}:{} requested an unknown service: {}", address, port, requested_service);
                return;
            }

            const auto complete_payload{this->wrap_with_ipc_header(data, ipc::types::message_type_t::SEND)};

            if (someip_header.message_type == someip::types::message_type_t::REQUEST) {
                m_pending_requests.emplace(this->makeRequestKey(someip_header), net::endpoint_t{address, port});
            }

            m_socket.send(m_apps.at(requested_service).handle, complete_payload, std::nullopt);
            m_logger->debug("Forwarded a SOME/IP payload to {}", m_apps.at(requested_service).app_name);

            break;
        }

        /* response from a remote service */
        case threesomeip::someip::types::message_type_t::RESPONSE: [[fallthrough]];
        case threesomeip::someip::types::message_type_t::ERROR: {
            const auto request_key = this->makeRequestKey(someip_header);

            if ( !m_pending_requests.contains(request_key)) {
                m_logger->warn("Reliable received a response without a registered request.");
                return;
            }

            if ( !std::holds_alternative<ipc::types::socket_handle_t>(m_pending_requests.at(request_key))) {
                m_logger->warn("Reliable received a response which is not for a local service.");
                m_pending_requests.erase(request_key);
                return;
            }

            const auto& requester_handle = std::get<ipc::types::socket_handle_t>(m_pending_requests.at(request_key));

            const auto complete_payload{this->wrap_with_ipc_header(data, ipc::types::message_type_t::SEND)};

            m_socket.send(requester_handle, complete_payload, std::nullopt);
            m_logger->debug("Forwarded a SOME/IP payload to {}", m_apps.at(requester_handle).app_name);

            m_pending_requests.erase(request_key);

            break;
        }

        default: {
            m_logger->warn("Unsupported SOME/IP message type: {}", static_cast<uint8_t>(someip_header.message_type));
        }
    }
}

/* socket_handle must not be passed from an internal data structure; its captured by reference */
void runtime_stub_t::evict_application(/* intentionally copied */ const application_entry_t app) {
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

auto runtime_stub_t::makeRequestKey(const someip::types::message_header_t& header) -> request_key_t {
    return request_key_t{
        .service_id{header.message_id.service_id},
        .method_id{header.message_id.method_id},
        .client_id{header.request_id.client_id},
        .session_id{header.request_id.session_id},
    };
}

std::vector<std::byte> runtime_stub_t::wrap_with_ipc_header(const std::span<const std::byte> data, ipc::types::message_type_t message_type) {
    std::vector<std::byte> message_buffer(someip::serdes::serialize_dry_run<ipc::types::message_header_t>() + data.size());

    ipc::types::message_header_t message_header{
        .start_of_frame{'#', 't', 'h', 'r', 'e', 'e', 's', 'o', 'm', 'e', 'i', 'p', '#'},
        .protocol_version{1},
        .message_type{message_type},
        ._flags{someip::types::uint8{0}},
        ._request_id{someip::types::uint16{0}},
        ._reserved{someip::types::uint16{0}},
        .payload_length{static_cast<someip::types::uint16>(data.size())},
    };

    size_t ipc_header_length = someip::serdes::serialize(message_buffer.data(), message_header);
    std::ranges::copy(data, message_buffer.begin() + ipc_header_length);

    return message_buffer;
}

} // namespace threesomeip::runtime