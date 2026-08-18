#ifndef _COMM_SOMEIP_HPP
#define _COMM_SOMEIP_HPP

#include <cstdint>
#include <cstddef>

namespace threesomeip::someip {

constexpr size_t MAX_PAYLOAD_SIZE = 1400;


enum class someip_message_type: uint8_t {
    REQUEST                     = 0x00,
    REQUEST_NO_RETURN           = 0x01,
    NOTIFICATION                = 0x02,
    TP_REQUEST                  = 0x20,
    TP_REQUEST_NO_RETURN        = 0x21,
    TP_NOTIFICATION             = 0x22,
    TP_RESPONSE                 = 0x23,
    TP_ERROR                    = 0x24,
    RESPONSE                    = 0x80,
    ERROR                       = 0x81
};

enum class someip_error_type: uint8_t {
    E_OK = 0x00,
    E_NOT_OK = 0x01,
    E_UNKNOWN_SERVICE = 0x02,
    E_UNKNOWN_METHOD = 0x03,
    E_NOT_READY = 0x04,
    E_NOT_REACHABLE = 0x05,
    E_TIMEOUT = 0x06,
    E_WRONG_PROTOCOL_VERSION = 0x07,
    E_WRONG_INTERFACE_VERSION = 0x08,
    E_MALFORMED_MESSAGE = 0x09,
    E_WRONG_MESSAGE_TYPE = 0x0A,
};

struct someip_message_header_t {
    struct {
        uint16_t service_id;    /* whose services are requested */
        uint16_t method_id;     /* their method* */
    } message_id;
    uint32_t length;
    struct {
        uint16_t client_id;     /* who requests the service */
        uint16_t session_id;    /* counter */
    } request_id;
    uint8_t protocol_version;
    uint8_t interface_version;
    someip_message_type message_type;
    uint8_t return_code;
};


} // threesomeip::someip


#endif // _COMM_SOMEIP_HPP