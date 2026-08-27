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

/*=============*\
 * APPLICATION *
\*=============*/
#include <runtime_proxy.hpp>
#include <serdes/serialization.hpp>
#include <serdes/someip_types.hpp>
#include <comm_ipc.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/
#include <spdlog/spdlog.h>


namespace fs = std::filesystem;


namespace threesomeip::runtime {
using namespace threesomeip;

runtime_proxy_t::runtime_proxy_t(
    utils::active_object_ptr_t active_object,
    const fs::path& sockets_path,
    std::string_view app_name,
    uint16_t app_id,
    std::string_view runtime_name,
    std::span<const config::service_configuration_t> offered_services,
    std::span<const config::service_configuration_t> requested_services
) noexcept:
    m_active_object(active_object),
    m_app_name(app_name),
    m_app_id(app_id),
    m_own_socket_handle((sockets_path / std::format("{}_{}.sock", m_app_name, m_app_id)).string()),
    m_runtime_handle((sockets_path / std::format("{}.sock", runtime_name)).string()),
    m_offered_services(offered_services.begin(), offered_services.end()),
    m_requested_services(requested_services.begin(), requested_services.end()),
    m_socket(m_active_object, m_own_socket_handle, std::bind_front(&runtime_proxy_t::handle_on_receive, this)) {

    auto& sinks = spdlog::get(std::string{m_active_object->get_name()})->sinks();
    m_logger = std::make_shared<spdlog::logger>("RTProxy", sinks.begin(), sinks.end());
#ifdef RUNTIME_COMM_DEBUG
    m_logger->set_level(spdlog::level::debug);
#else
    m_logger->set_level(spdlog::level::off);
#endif // RUNTIME_COMM_DEBUG
    spdlog::register_logger(m_logger);


    std::mutex _m;
    std::condition_variable _cv;

    /* returns if should sleep */
    const auto single_request = [&] (std::function<ipc::send_result_t(std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb)> request) -> bool {
        bool callback_triggered{false};
        ipc::send_result_t delayed_result{};

        const ipc::send_result_t request_result = request(
            [&] (const ipc::send_result_t result, [[maybe_unused]] const ipc::types::socket_handle_t& recipient, [[maybe_unused]] const std::span<const std::byte> data) {
                {
                    std::unique_lock<std::mutex> lock(_m);
                    callback_triggered = true;
                    delayed_result = result;
                }
                _cv.notify_all();
            }
        );

        std::unique_lock<std::mutex> lock(_m);
        switch (request_result) {
            case ipc::send_result_t::DELAYED_RESULT: {
                /* wait for callback */
                _cv.wait(
                    lock,
                    [&] {
                        return true == callback_triggered;
                    }
                );

                switch (delayed_result) {
                    case ipc::send_result_t::SENT: return false;
                    case ipc::send_result_t::RECIPIENT_AWAY: return true;
                    case ipc::send_result_t::SOCKET_DEAD: return true; /* here i will need to recover the socket */
                    default: assert(false && "Unreachable code"); return true;
                }
            }

            case ipc::send_result_t::RECIPIENT_AWAY: return true;
            case ipc::send_result_t::SOCKET_DEAD: return true; /* here i will need to recover the socket */
            default: return false;
        }
    };


    /* step by step initialization */
    while (true) {
        if (   !single_request(std::bind_front(&runtime_proxy_t::register_application, this))
            && !single_request(std::bind_front(&runtime_proxy_t::offer_services, this))
            && !single_request(std::bind_front(&runtime_proxy_t::request_services, this))) {

            break;
        }
        else {
            /* apply backoff; simple for now */
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

ipc::send_result_t runtime_proxy_t::register_application(std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb) {
    using namespace threesomeip;

    std::array<std::byte, ipc::MAX_PAYLOAD_SIZE> message_buffer{};

    /* calculate the size of the serialized header */
    constexpr size_t ipc_header_length{someip::serdes::serialize_dry_run<ipc::types::message_header_t>()};

    /* serialize the payload at an offset equal to the length of the header so we get the payload length */
    ipc::types::register_message_t message{
        .app_name{m_app_name},
        .app_id{m_app_id}
    };
    size_t payload_length = someip::serdes::serialize(message_buffer.data() + ipc_header_length, message);

    /* construct the header with the correct payload size and serialize it */
    ipc::types::message_header_t message_header{
        .start_of_frame{'#', 't', 'h', 'r', 'e', 'e', 's', 'o', 'm', 'e', 'i', 'p', '#'},
        .protocol_version{1},
        .message_type{ipc::types::message_type_t::REGISTER_APPLICATION},
        ._flags{someip::types::uint8{0}},
        ._request_id{someip::types::uint16{0}},
        ._reserved{someip::types::uint16{0}},
        .payload_length{static_cast<someip::types::uint16>(payload_length)},
    };
    someip::serdes::serialize(message_buffer.data(), message_header);

    m_logger->info("Announcing application registration");
    return m_socket.send(m_runtime_handle, std::span{message_buffer}.subspan(0, ipc_header_length + payload_length), std::move(delayed_cb));
}

ipc::send_result_t runtime_proxy_t::unregister_application(std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb) {
    using namespace threesomeip;

    std::array<std::byte, ipc::MAX_PAYLOAD_SIZE> message_buffer{};

    /* calculate the size of the serialized header */
    constexpr size_t ipc_header_length{someip::serdes::serialize_dry_run<ipc::types::message_header_t>()};

    /* serialize the payload at an offset equal to the length of the header so we get the payload length */
    ipc::types::unregister_message_t message{
        .app_name{m_app_name},
        .app_id{m_app_id}
    };
    size_t payload_length = someip::serdes::serialize(message_buffer.data() + ipc_header_length, message);

    /* construct the header with the correct payload size and serialize it */
    ipc::types::message_header_t message_header{
        .start_of_frame{'#', 't', 'h', 'r', 'e', 'e', 's', 'o', 'm', 'e', 'i', 'p', '#'},
        .protocol_version{1},
        .message_type{ipc::types::message_type_t::UNREGISTER_APPLICATION},
        ._flags{someip::types::uint8{0}},
        ._request_id{someip::types::uint16{0}},
        ._reserved{someip::types::uint16{0}},
        .payload_length{static_cast<someip::types::uint16>(payload_length)},
    };
    someip::serdes::serialize(message_buffer.data(), message_header);

    m_logger->info("Announcing application unregistration");
    return m_socket.send(m_runtime_handle, std::span{message_buffer}.subspan(0, ipc_header_length + payload_length), std::move(delayed_cb));
}

ipc::send_result_t runtime_proxy_t::offer_services(std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb) {
    using namespace threesomeip;

    std::array<std::byte, ipc::MAX_PAYLOAD_SIZE> message_buffer{};

    /* calculate the size of the serialized header */
    constexpr size_t ipc_header_length{someip::serdes::serialize_dry_run<ipc::types::message_header_t>()};

    /* serialize the payload at an offset equal to the length of the header so we get the payload length */
    ipc::types::offer_message_t message{m_offered_services};
    size_t payload_length = someip::serdes::serialize(message_buffer.data() + ipc_header_length, message);

    /* construct the header with the correct payload size and serialize it */
    ipc::types::message_header_t message_header{
        .start_of_frame{'#', 't', 'h', 'r', 'e', 'e', 's', 'o', 'm', 'e', 'i', 'p', '#'},
        .protocol_version{1},
        .message_type{ipc::types::message_type_t::OFFER_SERVICE},
        ._flags{someip::types::uint8{0}},
        ._request_id{someip::types::uint16{0}},
        ._reserved{someip::types::uint16{0}},
        .payload_length{static_cast<someip::types::uint16>(payload_length)},
    };
    someip::serdes::serialize(message_buffer.data(), message_header);

    m_logger->info("Announcing application's offered services");
    return m_socket.send(m_runtime_handle, std::span{message_buffer}.subspan(0, ipc_header_length + payload_length), std::move(delayed_cb));
}


ipc::send_result_t runtime_proxy_t::request_services(std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb) {
    using namespace threesomeip;

    std::array<std::byte, ipc::MAX_PAYLOAD_SIZE> message_buffer{};

    /* calculate the size of the serialized header */
    constexpr size_t ipc_header_length{someip::serdes::serialize_dry_run<ipc::types::message_header_t>()};

    /* serialize the payload at an offset equal to the length of the header so we get the payload length */
    ipc::types::request_message_t message{m_requested_services};
    size_t payload_length = someip::serdes::serialize(message_buffer.data() + ipc_header_length, message);

    /* construct the header with the correct payload size and serialize it */
    ipc::types::message_header_t message_header{
        .start_of_frame{'#', 't', 'h', 'r', 'e', 'e', 's', 'o', 'm', 'e', 'i', 'p', '#'},
        .protocol_version{1},
        .message_type{ipc::types::message_type_t::REQUEST_SERVICE},
        ._flags{someip::types::uint8{0}},
        ._request_id{someip::types::uint16{0}},
        ._reserved{someip::types::uint16{0}},
        .payload_length{static_cast<someip::types::uint16>(payload_length)},
    };
    someip::serdes::serialize(message_buffer.data(), message_header);

    m_logger->info("Announcing application's required services");
    return m_socket.send(m_runtime_handle, std::span{message_buffer}.subspan(0, ipc_header_length + payload_length), std::move(delayed_cb));
}

ipc::send_result_t runtime_proxy_t::send(std::span<const std::byte> someip_payload, std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb) {
    using namespace threesomeip;

    std::array<std::byte, ipc::MAX_PAYLOAD_SIZE> message_buffer{};

    /* construct the header and serialize it */
    ipc::types::message_header_t message_header{
        .start_of_frame{'#', 't', 'h', 'r', 'e', 'e', 's', 'o', 'm', 'e', 'i', 'p', '#'},
        .protocol_version{1},
        .message_type{ipc::types::message_type_t::SEND},
        ._flags{someip::types::uint8{0}},
        ._request_id{someip::types::uint16{0}},
        ._reserved{someip::types::uint16{0}},
        .payload_length{static_cast<someip::types::uint16>(someip_payload.size())},
    };
    size_t header_length = someip::serdes::serialize(message_buffer.data(), message_header);

    /* copy the already serialized payload containing the someip header and someip payload into the buffer */
    std::ranges::copy(someip_payload, message_buffer.begin() + header_length);

    m_logger->debug("Announcing application's intent to send a SOME/IP payload");
    return m_socket.send(m_runtime_handle, std::span{message_buffer}.subspan(0, header_length + someip_payload.size()), std::move(delayed_cb));
}

void runtime_proxy_t::register_message_listener(MessageReceivedCallback cb) {
    m_registered_listeners.emplace_back(std::move(cb));
    m_logger->info("A service or client has registered for message notifications");
}

void runtime_proxy_t::handle_on_receive(ipc::ud_socket_t& self, const ipc::types::socket_handle_t& sender, const std::span<const std::byte> data) noexcept {
    constexpr size_t ipc_header_length{someip::serdes::serialize_dry_run<ipc::types::message_header_t>()};

    std::byte* cursor{nullptr};
    const auto ipc_message_header = someip::serdes::deserialize<ipc::types::message_header_t>(data.data(), &cursor);

    if (ipc_message_header.protocol_version != someip::types::uint8{1}) return;
    if (ipc_message_header.message_type != ipc::types::message_type_t::SEND) return;

    for (const auto& callback: m_registered_listeners) {
        callback(data.subspan(ipc_header_length));
    }
}

} // namespace threesomeip
