/*=====*\
 * C++ *
\*=====*/
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <any>
#include <future>
#include <memory>
#include <ranges>
#include <span>

/*=============*\
 * APPLICATION *
\*=============*/
#include <calculator_proxy.hpp>
#include <serdes/someip_types.hpp>
#include <serdes/serialization.hpp>


namespace calculator {
using namespace threesomeip::someip::types;
using namespace threesomeip;


calculator_proxy_t::calculator_proxy_t(runtime::runtime_proxy_t& runtime_proxy):
    m_session_counter(0x0001), m_runtime_proxy(runtime_proxy) {
    runtime_proxy.register_message_listener(std::bind_front(&calculator_proxy_t::on_message, this));
}


std::future<float> calculator_proxy_t::add(float a, float b) {
    std::array<std::byte, 1400> someip_message_buffer{};
    constexpr size_t someip_header_length = someip::serdes::serialize_dry_run<message_header_t>();

    /* serialize function parameters */
    size_t arguments_size = someip::serdes::serialize(someip_message_buffer.data() + someip_header_length, a, b);

    /* construct the header with the correct payload size and serialize it */
    message_header_t message_header{
        .message_id{
            .service_id{SERVICE_ID},
            .method_id{ADD_METHOD_ID}
        },
        .length{static_cast<uint32>(8 + arguments_size)},
        .request_id{
            .client_id{m_runtime_proxy.get_id()},
            .session_id{m_session_counter}
        },
        .protocol_version{uint8{SOMEIP_PROTOCOL_VERSION}},
        .interface_version{uint8{INTERFACE_VERSION}},
        .message_type{message_type_t::REQUEST},
        .return_code{static_cast<uint8>(return_code_t::E_OK)}
    };
    (void) someip::serdes::serialize(someip_message_buffer.data(), message_header);

    auto promise = std::make_shared<std::promise<float>>();
    auto future = promise->get_future();
    m_pending_requests.emplace(m_session_counter, promise);

    m_runtime_proxy.send(std::span{someip_message_buffer.data(), someip_header_length + arguments_size}, std::nullopt);

    /* if max, go back to 1 */
    if (! ++m_session_counter) {
        ++m_session_counter;
    }

    return future;
}

void calculator_proxy_t::beepboop() {
    std::array<std::byte, 1400> someip_message_buffer{};
    constexpr size_t someip_header_length = someip::serdes::serialize_dry_run<message_header_t>();

    /* construct the header and serialize it */
    message_header_t message_header{
        .message_id{
            .service_id{SERVICE_ID},
            .method_id{BEEPBOOP_METHOD_ID}
        },
        .length{8},
        .request_id{
            .client_id{m_runtime_proxy.get_id()},
            .session_id{0x0000}
        },
        .protocol_version{uint8{SOMEIP_PROTOCOL_VERSION}},
        .interface_version{uint8{INTERFACE_VERSION}},
        .message_type{message_type_t::REQUEST_NO_RETURN},
        .return_code{static_cast<uint8>(return_code_t::E_OK)}
    };
    (void) someip::serdes::serialize(someip_message_buffer.data(), message_header);

    m_runtime_proxy.send(std::span{someip_message_buffer.data(), someip_header_length}, std::nullopt);
}

std::future<uint32_t> calculator_proxy_t::get_precision() {
    std::array<std::byte, 1400> someip_message_buffer{};
    constexpr size_t someip_header_length = someip::serdes::serialize_dry_run<message_header_t>();

    /* construct the header and serialize it */
    message_header_t message_header{
        .message_id{
            .service_id{SERVICE_ID},
            .method_id{GET_PRECISION_GETTER_ID}
        },
        .length{8},
        .request_id{
            .client_id{m_runtime_proxy.get_id()},
            .session_id{m_session_counter}
        },
        .protocol_version{uint8{SOMEIP_PROTOCOL_VERSION}},
        .interface_version{uint8{INTERFACE_VERSION}},
        .message_type{message_type_t::REQUEST},
        .return_code{static_cast<uint8>(return_code_t::E_OK)}
    };
    (void) someip::serdes::serialize(someip_message_buffer.data(), message_header);

    auto promise = std::make_shared<std::promise<uint32_t>>();
    auto future = promise->get_future();
    m_pending_requests.emplace(m_session_counter, promise);

    m_runtime_proxy.send(std::span{someip_message_buffer.data(), someip_header_length}, std::nullopt);

    /* if max, go back to 1 */
    if (! ++m_session_counter) {
        ++m_session_counter;
    }

    return future;
}

std::future<void> calculator_proxy_t::set_precision(uint32_t precision) {
    std::array<std::byte, 1400> someip_message_buffer{};
    constexpr size_t someip_header_length = someip::serdes::serialize_dry_run<message_header_t>();

    /* serialize function parameters */
    size_t arguments_size = someip::serdes::serialize(someip_message_buffer.data() + someip_header_length, precision);

    /* construct the header with the correct payload size and serialize it */
    message_header_t message_header{
        .message_id{
            .service_id{SERVICE_ID},
            .method_id{SET_PRECISION_SETTER_ID}
        },
        .length{static_cast<uint32>(8 + arguments_size)},
        .request_id{
            .client_id{m_runtime_proxy.get_id()},
            .session_id{m_session_counter}
        },
        .protocol_version{uint8{SOMEIP_PROTOCOL_VERSION}},
        .interface_version{uint8{INTERFACE_VERSION}},
        .message_type{message_type_t::REQUEST},
        .return_code{static_cast<uint8>(return_code_t::E_OK)}
    };
    (void) someip::serdes::serialize(someip_message_buffer.data(), message_header);

    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();
    m_pending_requests.emplace(m_session_counter, promise);

    m_runtime_proxy.send(std::span{someip_message_buffer.data(), someip_header_length + arguments_size}, std::nullopt);

    /* if max, go back to 1 */
    if (! ++m_session_counter) {
        ++m_session_counter;
    }

    return future;
}

void calculator_proxy_t::on_message(std::span<const std::byte> payload) {

    std::byte* cursor{nullptr};
    message_header_t someip_header = someip::serdes::deserialize<message_header_t>(payload.data(), &cursor);

    if (someip_header.message_id.service_id != SERVICE_ID) return; /* message not intended for this service */
    if (!std::ranges::contains(METHOD_IDS, someip_header.message_id.method_id)) return;
    if (SOMEIP_PROTOCOL_VERSION != someip_header.protocol_version) return;
    if (INTERFACE_VERSION != someip_header.interface_version) return;
    if (!m_pending_requests.contains(someip_header.request_id.session_id)) return;
    if (someip_header.message_id.method_id == BEEPBOOP_METHOD_ID) return;


    switch (someip_header.message_type) {
        case message_type_t::RESPONSE: {
            switch(someip_header.message_id.method_id) {
                case ADD_METHOD_ID: {
                    const auto return_value = someip::serdes::deserialize<float32>(cursor);
                    auto promise = std::static_pointer_cast<std::promise<float>>(m_pending_requests.at(someip_header.request_id.session_id));
                    m_pending_requests.erase(someip_header.request_id.session_id);
                    promise->set_value(return_value);
                    break;
                }

                case GET_PRECISION_GETTER_ID: {
                    const auto return_value = someip::serdes::deserialize<uint32>(cursor);
                    auto promise = std::static_pointer_cast<std::promise<uint32_t>>(m_pending_requests.at(someip_header.request_id.session_id));
                    m_pending_requests.erase(someip_header.request_id.session_id);
                    promise->set_value(return_value);
                    break;
                }

                case SET_PRECISION_SETTER_ID: {
                    auto promise = std::static_pointer_cast<std::promise<void>>(m_pending_requests.at(someip_header.request_id.session_id));
                    m_pending_requests.erase(someip_header.request_id.session_id);
                    promise->set_value();
                    break;
                }

                default: {return;}
            }
            break;
        }
        case message_type_t::ERROR: {
            /* todo */
            break;
        }
        default: return;
    }
}



} // namespace calculator
