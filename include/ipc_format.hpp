#ifndef _IPC_FORMAT_HPP
#define _IPC_FORMAT_HPP

/*=====*\
 * C++ *
\*=====*/
#include <string>
#include <cstdint>
#include <variant>
#include <utility>


namespace threesomeip::ipc {


constexpr uint16_t MAX_PAYLOAD_SIZE = 1024;


enum class message_type_t: uint8_t {
    REGISTER_APPLICATION = 0,
    UNREGISTER_APPLICATION,
    OFFER_SERVICE,
    REQUEST_SERVICE,
};

struct __attribute__((packed)) ipc_message_header_t {
    std::byte start_of_frame[13];
    uint8_t protocol_version;
    message_type_t message_type;
    uint8_t _flags;
    uint16_t _request_id;
    uint16_t _reserved;
    uint16_t payload_length;
};

using socket_handle_t = std::string;

} // namespace threesomeip::ipc
#endif // _IPC_FORMAT_HPP