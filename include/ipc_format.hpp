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
    uint8_t flags;
    uint16_t request_id;
    uint16_t payload_length;
    uint16_t reserved;
};

void encodeRegisterApplicationRequest();
void encodeUnregisterApplicationRequest();
void encodeOfferServiceRequest();
void encodeRequestServiceRequest();


} // namespace threesomeip::ipc
#endif _IPC_FORMAT_HPP