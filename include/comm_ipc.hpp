#ifndef _COMM_IPC_HPP
#define _COMM_IPC_HPP

/*=====*\
 * C++ *
\*=====*/
#include <string>

/*===========*\
 * FRAMEWORK *
\*===========*/
#include <configuration.hpp>
#include <serdes/someip_types.hpp>


namespace threesomeip::ipc {


constexpr uint16_t MAX_PAYLOAD_SIZE = 1024;


namespace types {
using namespace threesomeip;


using socket_handle_t = std::string;


/*
    SERIALIZED TYPES
*/
enum class message_type_t: uint8_t {
    REGISTER_APPLICATION = 0,
    UNREGISTER_APPLICATION,
    OFFER_SERVICE,
    REQUEST_SERVICE,
    INVOKE
};

struct message_header_t {
    someip::types::flstring_utf8<13> start_of_frame;
    someip::types::uint8 protocol_version;
    message_type_t message_type;
    someip::types::uint8 _flags;
    someip::types::uint16 _request_id;
    someip::types::uint16 _reserved;
    someip::types::uint16 payload_length;
};

struct register_message_t {
    someip::types::dlstring_utf8 app_name;
    someip::types::uint16 app_id;
};

using unregister_message_t = register_message_t;

using offer_message_t = someip::types::dlarray<config::service_configuration_t>;
using request_message_t = offer_message_t;

} // namespace types

} // namespace threesomeip::ipc
#endif // _COMM_IPC_HPP