#ifndef _IPC_FORMAT_HPP
#define _IPC_FORMAT_HPP

/*=====*\
 * C++ *
\*=====*/
#include <string>
#include <cstdint>
#include <variant>
#include <utility>
#include <array>

/*=====*\
 * C++ *
\*=====*/
#include <serialization.hpp>


namespace threesomeip::ipc {

using socket_handle_t = std::string;


constexpr uint16_t MAX_PAYLOAD_SIZE = 1024;


enum class message_type_t: uint8_t {
    REGISTER_APPLICATION = 0,
    UNREGISTER_APPLICATION,
    OFFER_SERVICE,
    REQUEST_SERVICE,
};

struct ipc_message_header_t {
    std::array<std::byte, 13> start_of_frame;
    uint8_t protocol_version;
    message_type_t message_type;
    uint8_t _flags;
    uint16_t _request_id;
    uint16_t _reserved;
    uint16_t payload_length;
};

constexpr size_t SERIALIZED_MESSAGE_HEADER_SIZE{sizeof(ipc_message_header_t) + /* array length prefix */ sizeof(uint16_t)};


} // namespace threesomeip::ipc
#endif // _IPC_FORMAT_HPP