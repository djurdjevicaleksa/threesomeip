#ifndef _SERDES_TYPES_HPP
#define _SERDES_TYPES_HPP

/*=====*\
 * C++ *
\*=====*/
#include <cstdint>
#include <string>
#include <array>
#include <string_view>


namespace threesomeip::someip::types {

using sbool = bool;

using sint8 = int8_t;
using sint16 = int16_t;
using sint32 = int32_t;
using sint64 = int64_t;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using float32 = float;
using float64 = double;


using dlstring_utf8 = std::string;

template<size_t N>
using flstring_utf8 = std::array<char, N>;

template<typename T>
using dlarray = std::vector<T>;

template<typename T, size_t N>
using flarray = std::array<T, N>;


enum class message_type_t: uint8_t {
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

enum class return_code_t: uint8_t {
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

struct message_header_t {
    struct {
        uint16 service_id;    /* whose services are requested */
        uint16 method_id;     /* their method* */
    } message_id;
    uint32 length;
    struct {
        uint16 client_id;     /* who requests the service */
        uint16 session_id;    /* counter */
    } request_id;
    uint8 protocol_version;
    uint8 interface_version;
    message_type_t message_type;
    uint8 return_code;

    bool operator==(const message_header_t&) const = default;
};

} // threesomeip::someip::types


#endif // _SERDES_TYPES_HPP