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
#include <ranges>
#include <vector>
#include <span>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cassert>
#include <thread>
#include <optional>

/*=============*\
 * APPLICATION *
\*=============*/
#include <runtime_proxy.hpp>
#include <serdes/serialization.hpp>
#include <serdes/someip_types.hpp>
#include <comm_ipc.hpp>
#include <timer.hpp>
#include <awaitable.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/
#include <spdlog/spdlog.h>


namespace fs = std::filesystem;


namespace threesomeip::runtime {
using namespace threesomeip;

runtime_proxy_t::runtime_proxy_t(
    fs::path configuration_path,
    utils::active_object_ptr_t active_object,
    std::string_view app_name,
    uint16_t app_id,
    std::span<const config::service_configuration_t> offered_services,
    std::span<const config::service_configuration_t> requested_services
) noexcept:
    configurable_t(configuration_path),
    m_active_object(active_object),
    m_app_name(app_name),
    m_app_id(app_id),
    m_own_socket_handle((m_ecu_configuration.sockets_path / std::format("{}_{}.sock", m_app_name, m_app_id)).string()),
    m_runtime_handle((m_ecu_configuration.sockets_path / std::format("{}.sock", m_ecu_configuration.runtime_application_name)).string()),
    m_offered_services(offered_services.begin(), offered_services.end()),
    m_requested_services(requested_services.begin(), requested_services.end()),
    m_socket(m_active_object, m_own_socket_handle, std::bind_front(&runtime_proxy_t::handle_on_receive, this)),
    m_runtime_online(false),
    m_heartbeat(utils::timer_factory::make_periodic_timer(m_active_object, std::chrono::seconds(3), std::bind_front(&runtime_proxy_t::send_heartbeat, this))) {

    auto& sinks = spdlog::get(std::string{m_active_object->get_name()})->sinks();
    m_logger = std::make_shared<spdlog::logger>("RTProxy", sinks.begin(), sinks.end());
#ifdef RUNTIME_COMM_DEBUG
    m_logger->set_level(spdlog::level::debug);
#else
    m_logger->set_level(spdlog::level::off);
#endif // RUNTIME_COMM_DEBUG
    spdlog::register_logger(m_logger);


    m_reconnect_in_progress = true;
    this->reconnect(
        [this] (bool success) {
            m_reconnect_in_progress = false;
            if (success) {
                m_runtime_online = true;
                m_heartbeat->start();
            }
        }
    );
}

utils::detached_task_t runtime_proxy_t::reconnect(std::function<void(bool)> on_done) {
    if (co_await this->register_application() != ipc::send_result_t::SENT) {
        on_done(false);
        co_return;
    }
    if (co_await this->offer_services() != ipc::send_result_t::SENT) {
        on_done(false);
        co_return;
    }
    if (co_await this->request_services() != ipc::send_result_t::SENT) {
        on_done(false);
        co_return;
    }

    on_done(true);
    co_return;
}

void runtime_proxy_t::send_heartbeat() {
    auto complete_payload = this->wrap_with_ipc_header({}, ipc::types::message_type_t::HEARTBEAT);

    /* this lambda can run both synchronously and asynchronously */
    const auto handleResult = [this] (const ipc::send_result_t result) {
        switch(result) {
            /* connect only when it was previously disconnected */
            case ipc::send_result_t::SENT: {
                if (!m_runtime_online && !m_reconnect_in_progress) {
                    m_reconnect_in_progress = true;
                    this->reconnect(
                        [this] (bool success) {
                            m_reconnect_in_progress = false;
                            if (success) {
                                m_runtime_online = true;
                                m_heartbeat->start();
                            }
                        }
                    );
                }
                break;
            }

            /* can only happen on the first try */
            case ipc::send_result_t::DELAYED_RESULT: {
                /* do nothing, another call of this lambda will act accordingly */
                break;
            }

            /* same for both calls to this lambda */
            case ipc::send_result_t::RECIPIENT_AWAY: {
                if (m_runtime_online) {
                    m_logger->debug("The runtime went offline");
                }
                m_runtime_online = false;
                break;
            }

            case ipc::send_result_t::SOCKET_DEAD: {
                /* assuming wont happen */
                break;
            }
        }
    };

    handleResult(
        m_socket.send(
            m_runtime_handle,
            complete_payload,
            [handleResult] (const ipc::send_result_t result, const ipc::types::socket_handle_t& recipient, const std::span<const std::byte> data) -> void {
                (void) recipient;
                (void) data;
                handleResult(result);
            }
        )
    );

    m_logger->debug("Heartbeat");
}

utils::awaitable_t<ipc::send_result_t> runtime_proxy_t::register_application() {
    const ipc::types::unregister_message_t payload{.app_name{m_app_name}, .app_id{m_app_id}};
    std::vector<std::byte> buffer(someip::serdes::serialize_dry_run(payload));
    someip::serdes::serialize(buffer.data(), payload);

    auto complete_payload = this->wrap_with_ipc_header(buffer, ipc::types::message_type_t::REGISTER_APPLICATION);

    m_logger->info("Announcing application registration");

    return utils::awaitable_t<ipc::send_result_t>(
        [this, msg = std::move(complete_payload)] (std::function<void(ipc::send_result_t)> resume) {
            auto result = m_socket.send(
                m_runtime_handle,
                msg,
                [resume] (const ipc::send_result_t result_, const ipc::types::socket_handle_t& recipient, const std::span<const std::byte> data) {
                    (void) recipient;
                    (void) data;

                    resume(result_);
                }
            );

            if (result != ipc::send_result_t::DELAYED_RESULT) {
                resume(result);
            }
        }
    );
}

utils::awaitable_t<ipc::send_result_t> runtime_proxy_t::unregister_application() {
    const ipc::types::unregister_message_t payload{.app_name{m_app_name}, .app_id{m_app_id}};
    std::vector<std::byte> buffer(someip::serdes::serialize_dry_run(payload));
    someip::serdes::serialize(buffer.data(), payload);

    auto complete_payload = this->wrap_with_ipc_header(buffer, ipc::types::message_type_t::UNREGISTER_APPLICATION);

    m_logger->info("Announcing application unregistration");

    return utils::awaitable_t<ipc::send_result_t>(
        [this, msg = std::move(complete_payload)] (std::function<void(ipc::send_result_t)> resume) {
            auto result = m_socket.send(
                m_runtime_handle,
                msg,
                [resume] (const ipc::send_result_t result_, const ipc::types::socket_handle_t& recipient, const std::span<const std::byte> data) {
                    (void) recipient;
                    (void) data;

                    resume(result_);
                }
            );

            if (result != ipc::send_result_t::DELAYED_RESULT) {
                resume(result);
            }
        }
    );
}

utils::awaitable_t<ipc::send_result_t> runtime_proxy_t::offer_services() {
    const ipc::types::request_message_t payload{m_offered_services};
    std::vector<std::byte> buffer(someip::serdes::serialize_dry_run(payload));
    someip::serdes::serialize(buffer.data(), payload);

    auto complete_payload = this->wrap_with_ipc_header(buffer, ipc::types::message_type_t::OFFER_SERVICE);

    m_logger->info("Announcing application's offered services");

    return utils::awaitable_t<ipc::send_result_t>(
        [this, msg = std::move(complete_payload)] (std::function<void(ipc::send_result_t)> resume) {
            auto result = m_socket.send(
                m_runtime_handle,
                msg,
                [resume] (const ipc::send_result_t result_, const ipc::types::socket_handle_t& recipient, const std::span<const std::byte> data) {
                    (void) recipient;
                    (void) data;

                    resume(result_);
                }
            );

            if (result != ipc::send_result_t::DELAYED_RESULT) {
                resume(result);
            }
        }
    );
}

utils::awaitable_t<ipc::send_result_t> runtime_proxy_t::request_services() {
    const ipc::types::request_message_t payload{m_requested_services};
    std::vector<std::byte> buffer(someip::serdes::serialize_dry_run(payload));
    someip::serdes::serialize(buffer.data(), payload);

    auto complete_payload = this->wrap_with_ipc_header(buffer, ipc::types::message_type_t::REQUEST_SERVICE);

    m_logger->info("Announcing application's required services");

    return utils::awaitable_t<ipc::send_result_t>(
        [this, msg = std::move(complete_payload)] (std::function<void(ipc::send_result_t)> resume) {
            auto result = m_socket.send(
                m_runtime_handle,
                msg,
                [resume] (const ipc::send_result_t result_, const ipc::types::socket_handle_t& recipient, const std::span<const std::byte> data) {
                    (void) recipient;
                    (void) data;

                    resume(result_);
                }
            );

            if (result != ipc::send_result_t::DELAYED_RESULT) {
                resume(result);
            }
        }
    );
}

ipc::send_result_t runtime_proxy_t::send(std::span<const std::byte> someip_payload, std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb) {
    const auto complete_payload = this->wrap_with_ipc_header(someip_payload, ipc::types::message_type_t::SEND);
    m_logger->debug("Sending a SOME/IP payload");
    return m_socket.send(m_runtime_handle, complete_payload, std::move(delayed_cb));
}

void runtime_proxy_t::register_message_listener(MessageReceivedCallback cb) {
    m_registered_listeners.emplace_back(std::move(cb));
    m_logger->info("A service or client has registered for message notifications");
}

void runtime_proxy_t::handle_on_receive(ipc::ud_socket_t& self, const ipc::types::socket_handle_t& sender, const std::span<const std::byte> data) noexcept {
    (void) self;
    (void) sender;

    constexpr size_t ipc_header_length{someip::serdes::serialize_dry_run<ipc::types::message_header_t>()};

    std::byte* cursor{nullptr};
    const auto ipc_message_header = someip::serdes::deserialize<ipc::types::message_header_t>(data.data(), &cursor);

    if (ipc_message_header.protocol_version != someip::types::uint8{1}) return;
    if (ipc_message_header.message_type != ipc::types::message_type_t::SEND) return;

    for (const auto& callback: m_registered_listeners) {
        callback(data.subspan(ipc_header_length));
    }
}

std::vector<std::byte> runtime_proxy_t::wrap_with_ipc_header(const std::span<const std::byte> data, ipc::types::message_type_t message_type) const {
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

} // namespace threesomeip
