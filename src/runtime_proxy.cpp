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
#include <serialization.hpp>

/*===========*\
 * 3RD PARTY *
\*===========*/


namespace fs = std::filesystem;


namespace threesomeip::runtime {
using namespace threesomeip;

runtime_proxy_t::runtime_proxy_t(
    const fs::path& sockets_path,
    std::string_view app_name,
    uint16_t app_id,
    std::string_view runtime_name,
    std::span<const config::service_configuration_t> offered_services,
    std::span<const config::service_configuration_t> requested_services
) noexcept:
    m_app_name(app_name),
    m_app_id(app_id),
    m_own_socket_handle((sockets_path / std::format("{}_{}.sock", m_app_name, m_app_id)).string()),
    m_runtime_handle((sockets_path / std::format("{}.sock", runtime_name)).string()),
    m_offered_services(offered_services.begin(), offered_services.end()),
    m_requested_services(requested_services.begin(), requested_services.end()),
    m_socket(m_own_socket_handle, std::bind_front(&runtime_proxy_t::handle_on_receive, this)) {

    std::mutex _m;
    std::condition_variable _cv;

    /* returns if should sleep */
    const auto single_request = [&] (std::function<ipc::send_result_t(std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb)> request) -> bool {
        bool callback_triggered{false};
        ipc::send_result_t delayed_result{};

        const ipc::send_result_t result = request(
            [&] (const ipc::send_result_t result, [[maybe_unused]] const ipc::socket_handle_t& recipient, [[maybe_unused]] const std::span<const std::byte> data) {
                {
                    std::unique_lock<std::mutex> lock(_m);
                    callback_triggered = true;
                    delayed_result = result;
                }
                _cv.notify_all();
            }
        );

        std::unique_lock<std::mutex> lock(_m);
        switch (result) {
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
        if (   !single_request(std::bind_front(&runtime_proxy_t::registerApplication, this))
            && !single_request(std::bind_front(&runtime_proxy_t::offerServices, this))
            && !single_request(std::bind_front(&runtime_proxy_t::requestServices, this))) {

            break;
        }
        else {
            /* apply backoff; simple for now */
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

ipc::send_result_t runtime_proxy_t::registerApplication(std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb) {
    using namespace threesomeip::ipc;

    std::array<std::byte, MAX_PAYLOAD_SIZE> message_buffer{};

    /* calculate the size of the serialized header */
    const size_t header_length{serdes::serialize_dry_run(threesomeip::ipc::ipc_message_header_t{})};

    /* construct the payload and serialize it */
    threesomeip::ipc::ipc_register_message_t payload{
        .application_name{m_app_name},
        .application_id{m_app_id}
    };

    size_t payload_length = serdes::serialize(message_buffer.data() + header_length, payload);

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

    return m_socket.send(m_runtime_handle, std::span{message_buffer}.subspan(0, header_length + payload_length), std::move(delayed_cb));
}

ipc::send_result_t runtime_proxy_t::unregisterApplication(std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb) {
    using namespace threesomeip::ipc;

    std::array<std::byte, MAX_PAYLOAD_SIZE> message_buffer{};

    /* calculate the size of the serialized header */
    const size_t header_length{serdes::serialize_dry_run(threesomeip::ipc::ipc_message_header_t{})};

    /* construct the payload and serialize it */
    threesomeip::ipc::ipc_unregister_message_t payload{
        .application_name{m_app_name},
        .application_id{m_app_id}
    };

    size_t payload_length = serdes::serialize(message_buffer.data() + header_length, payload);

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

    return m_socket.send(m_runtime_handle, std::span{message_buffer}.subspan(0, header_length + payload_length), std::move(delayed_cb));
}

ipc::send_result_t runtime_proxy_t::offerServices(std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb) {
    using namespace threesomeip::ipc;

    std::array<std::byte, MAX_PAYLOAD_SIZE> message_buffer{};

    /* calculate the size of the serialized header */
    const size_t header_length{serdes::serialize_dry_run(threesomeip::ipc::ipc_message_header_t{})};

    /* construct the payload and serialize it */
    threesomeip::ipc::ipc_offer_services_message_t payload{
        m_offered_services
    };

    size_t payload_length = serdes::serialize(message_buffer.data() + header_length, payload);

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

    return m_socket.send(m_runtime_handle, std::span{message_buffer}.subspan(0, header_length + payload_length), std::move(delayed_cb));
}


ipc::send_result_t runtime_proxy_t::requestServices(std::optional<ipc::ud_socket_t::DelayedResultCallback> delayed_cb) {
    using namespace threesomeip::ipc;

    std::array<std::byte, MAX_PAYLOAD_SIZE> message_buffer{};

    /* calculate the size of the serialized header */
    const size_t header_length{serdes::serialize_dry_run(threesomeip::ipc::ipc_message_header_t{})};

    /* construct the payload and serialize it */
    threesomeip::ipc::ipc_request_services_message_t payload{
        m_requested_services
    };

    size_t payload_length = serdes::serialize(message_buffer.data() + header_length, payload);

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
        .message_type{threesomeip::ipc::message_type_t::REQUEST_SERVICE},
        ._flags{static_cast<uint8_t>(0)},
        ._request_id{static_cast<uint16_t>(0)},
        ._reserved{static_cast<uint16_t>(0)},
        .payload_length{static_cast<uint16_t>(payload_length)},
    };
    serdes::serialize(message_buffer.data(), message_header);

    return m_socket.send(m_runtime_handle, std::span{message_buffer}.subspan(0, header_length + payload_length), std::move(delayed_cb));
}


} // namespace threesomeip
