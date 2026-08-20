#ifndef _CALCULATOR_STUB_HPP
#define _CALCULATOR_STUB_HPP

#include <cstdint>
#include <span>

#include <comm_someip.hpp>
#include <runtime_proxy.hpp>


namespace calculator {

class calculator_stub_t {
private:
    static constexpr uint16_t SERVICE_ID = 0x1234;
    static constexpr uint16_t ADD_METHOD_ID = 0x0000;
    static constexpr uint16_t BEEPBOOP_METHOD_ID = 0x0001;
    static constexpr uint16_t GET_PRECISION_GETTER_ID = 0x0002;
    static constexpr uint16_t SET_PRECISION_SETTER_ID = 0x0003;
    static constexpr uint16_t SOMEIP_PROTOCOL_VERSION = 0x01;
    static constexpr uint16_t INTERFACE_VERSION = 0x01;

public:

    /* request-response */
    virtual float on_add(float a, float b) const = 0;

    /* fire-and-forget */
    virtual void on_beepboop() const = 0;

    /* field getter */
    virtual uint32_t on_get_precision() const = 0;

    /* field setter */
    virtual void on_set_precision(uint32_t) = 0;

protected:

    void on_message(const threesomeip::someip::someip_message_header_t& someip_header, std::span<const std::byte>) {
        if (someip_header.service_id != SERVICE_ID) return; /* message not intended for this service */

        if 


        switch (someip_header.message_type) {
            case threesomeip::someip::someip_message_type::REQUEST: {

                threesomeip::someip::someip_message_header_t{
                    .message_id = someip_header.message_id,
                    .length = 0,
                    .request_id = someip_header.request_id,
                    .protocol_version = SOMEIP_PROTOCOL_VERSION,
                    .interface_version = INTERFACE_VERSION,
                    .
                }

                switch ()


            }
        }

        if (someip_header.message_type == threesomeip::someip::someip_message_type::REQUEST)

            
        }

        someip_header.

        switch (someip_header.method_id) {
            case ADD_METHOD_ID: {

            }
        }


    }

private:

    threesomeip::runtime::runtime_proxy_t& m_runtime_proxy;


};

} // namespace calculator

#endif // _CALCULATOR_STUB_HPP

