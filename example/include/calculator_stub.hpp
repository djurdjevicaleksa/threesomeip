#ifndef _CALCULATOR_STUB_HPP
#define _CALCULATOR_STUB_HPP

/*=====*\
 * C++ *
\*=====*/
#include <cstdint>
#include <span>
#include <ranges>
#include <array>
#include <optional>
#include <cstddef>
#include <cstdint>
#include <functional>

/*=============*\
 * APPLICATION *
\*=============*/
#include <runtime_proxy.hpp>
#include <serdes/someip_types.hpp>
#include <serdes/serialization.hpp>

namespace calculator {
using namespace threesomeip::someip::types;
using namespace threesomeip;


class calculator_stub_t {
private:
    static constexpr uint16_t SERVICE_ID = 0x1234;
    static constexpr uint16_t ADD_METHOD_ID = 0x0000;
    static constexpr uint16_t BEEPBOOP_METHOD_ID = 0x0001;
    static constexpr uint16_t GET_PRECISION_GETTER_ID = 0x0002;
    static constexpr uint16_t SET_PRECISION_SETTER_ID = 0x0003;
    static constexpr uint16_t SOMEIP_PROTOCOL_VERSION = 0x01;
    static constexpr uint16_t INTERFACE_VERSION = 0x01;
    static constexpr std::array METHOD_IDS{ADD_METHOD_ID, BEEPBOOP_METHOD_ID, GET_PRECISION_GETTER_ID, SET_PRECISION_SETTER_ID};

public:

    calculator_stub_t(runtime::runtime_proxy_t& runtime_proxy): m_runtime_proxy(runtime_proxy) {
        m_runtime_proxy.register_message_listener(std::bind_front(&calculator_stub_t::on_message, this));
    }

    virtual ~calculator_stub_t() = default;

    /* request-response */
    virtual float on_add(float a, float b) const = 0;

    /* fire-and-forget */
    virtual void on_beepboop() const = 0;

    /* field getter */
    virtual uint32_t on_get_precision() const = 0;

    /* field setter */
    virtual void on_set_precision(uint32_t precision) = 0;

protected:

    void on_message(std::span<const std::byte> payload) {
        std::byte* cursor{nullptr};
        const auto someip_header = someip::serdes::deserialize<message_header_t>(payload.data(), &cursor);

        if (someip_header.message_id.service_id != SERVICE_ID) return; /* message not intended for this service */

        std::array<std::byte, 1400> someip_message_buffer{};
        std::span<const std::byte> someip_message_view{};

        message_header_t response_header = someip_header;

        do {
            if (!std::ranges::contains(METHOD_IDS, someip_header.message_id.method_id)) {
                response_header.message_type = message_type_t::ERROR;
                response_header.return_code = static_cast<uint8>(return_code_t::E_UNKNOWN_METHOD);
                response_header.length = 8; /* no payload */
                size_t someip_header_length = someip::serdes::serialize(someip_message_buffer.data(), response_header);

                someip_message_view = std::span{someip_message_buffer.data(), someip_header_length};
                break;
            }
            else if (SOMEIP_PROTOCOL_VERSION != someip_header.protocol_version) {
                response_header.message_type = message_type_t::ERROR;
                response_header.return_code = static_cast<uint8>(return_code_t::E_WRONG_PROTOCOL_VERSION);
                response_header.length = 8; /* no payload */
                size_t someip_header_length = someip::serdes::serialize(someip_message_buffer.data(), response_header);

                someip_message_view = std::span{someip_message_buffer.data(), someip_header_length};
                break;
            }
            else if (INTERFACE_VERSION != someip_header.interface_version) {
                response_header.message_type = message_type_t::ERROR;
                response_header.return_code = static_cast<uint8>(return_code_t::E_WRONG_INTERFACE_VERSION);
                response_header.length = 8; /* no payload */
                size_t someip_header_length = someip::serdes::serialize(someip_message_buffer.data(), response_header);

                someip_message_view = std::span{someip_message_buffer.data(), someip_header_length};
                break;
            }
            else {
                switch (someip_header.message_id.method_id) {
                    case ADD_METHOD_ID: {
                        const auto a = someip::serdes::deserialize<float32>(cursor, &cursor);
                        const auto b = someip::serdes::deserialize<float32>(cursor);
                        const auto return_value = this->on_add(a, b);

                        constexpr size_t someip_header_length = someip::serdes::serialize_dry_run<message_header_t>();

                        size_t return_value_size = someip::serdes::serialize(someip_message_buffer.data() + someip_header_length, return_value);

                        response_header.message_type = message_type_t::RESPONSE;
                        response_header.return_code = static_cast<uint8>(return_code_t::E_OK);
                        response_header.length = 8 + return_value_size;
                        (void) someip::serdes::serialize(someip_message_buffer.data(), response_header);

                        someip_message_view = std::span{someip_message_buffer.data(), someip_header_length + return_value_size};
                        break;
                    }

                    case BEEPBOOP_METHOD_ID: {
                        this->on_beepboop();
                        return;
                    }

                    case GET_PRECISION_GETTER_ID: {
                        const auto return_value = this->on_get_precision();

                        constexpr size_t someip_header_length = someip::serdes::serialize_dry_run<message_header_t>();

                        size_t return_value_size = someip::serdes::serialize(someip_message_buffer.data() + someip_header_length, return_value);

                        response_header.message_type = message_type_t::RESPONSE;
                        response_header.return_code = static_cast<uint8>(return_code_t::E_OK);
                        response_header.length = 8 + return_value_size;
                        (void) someip::serdes::serialize(someip_message_buffer.data(), response_header);

                        someip_message_view = std::span{someip_message_buffer.data(), someip_header_length + return_value_size};
                        break;
                    }

                    case SET_PRECISION_SETTER_ID: {
                        const auto precision = someip::serdes::deserialize<uint32>(cursor);
                        this->on_set_precision(precision);

                        response_header.message_type = message_type_t::RESPONSE;
                        response_header.return_code = static_cast<uint8>(return_code_t::E_OK);
                        response_header.length = 8; /* no payload */
                        size_t someip_header_length = someip::serdes::serialize(someip_message_buffer.data(), response_header);

                        someip_message_view = std::span{someip_message_buffer.data(), someip_header_length};
                        break;
                    }
                }
            }
        } while (0);

        if (message_type_t::REQUEST == someip_header.message_type) {
            m_runtime_proxy.send(someip_message_view, std::nullopt);
        }
    }

protected:

    runtime::runtime_proxy_t& m_runtime_proxy;
};

} // namespace calculator

#endif // _CALCULATOR_STUB_HPP

