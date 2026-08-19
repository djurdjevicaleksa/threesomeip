#ifndef _SERDES_CONFIGURATION_HPP
#define _SERDES_CONFIGURATION_HPP

#include <cstddef>
#include <bit>

/*
    [PRS_SOMEIP_00004] (De)serialization parameters - generated per-interface; global for now.
*/

namespace threesomeip::ipc::serdes {

/*
    Defines the alignment requirement for the data element immediately
    following a variable length data element (if it is not the last element in
    the serialized data stream) in units of bytes.

    Allowed range or values: [1], 2, 4, 8, 16, 32
*/
constexpr size_t SERDES_ALIGNMENT = 1;

/*
    Defines the byte order of the payload message.

    Allowed range or values: [big], little, *OPAQUE*
*/
constexpr std::endian SERDES_BYTE_ORDER = std::endian::big;

/*
    This parameter shall be used to determine the size of the length field
    based on wire type in the context of using the TLV encoding. If set to TRUE,
    the size of the length field defined in the data definition shall be ignored
    and the size of the length field shall be selected according to the wire type.

    Allowed range or values: [true], false
*/
constexpr bool SERDES_IS_DYNAMIC_LENGTH_FIELD_SIZE = true;

/*
    This parameter shall be used to determine the size of the length field based on
    wire type in the context of using the TLV encoding. If set to TRUE, the size of
    the length field defined in the data definition shall be ignored and the size
    of the length field shall be selected according to the wire type.

    Allowed range or values:
        - 0, 1, 2, [4] for fixed length arrays where 0 means no length field present
        - 1, 2, [4] for dynamic length fields where length field is mandatory
*/
constexpr size_t SERDES_SIZE_OF_ARRAY_LENGTH_FIELD = 4;

/*
    Defines the size of the length field (in bytes) that swill be put in front of
    a string in the SOME/IP message.

    Allowed range or values:
        - 0, 1, 2, [4] for fixed length strings where 0 means no length fields present
        - 1, 2, [4] for dynamic length strings where length field is mandatory
*/
constexpr size_t SERDES_SIZE_OF_STRING_LENGTH_FIELD = 4;

/*
    Defines the size of the length field (in bytes) that will be put in front of
    a struct in the SOME/IP message.

    Allowed range or values: [0], 1, 2, 4
*/
constexpr size_t SERDES_SIZE_OF_STRUCT_LENGTH_FIELD = 0;

/*
    Defines the size of the length field (in bytes) that will be put in front of
    a union in the SOME/IP message.

    Allowed range or values: 0, 1, 2, [4]
*/
constexpr size_t SERDES_SIZE_OF_UNION_LENGTH_FIELD = 4;

/*
    Defines the size of the payload selector field (in bytes) that will be put in
    front of a union in the SOME/IP message.

    Allowed range of values: 1, 2, [4]
*/
constexpr size_t SERDES_SIZE_OF_UNION_TYPE_SELECTOR_FIELD = 4;

/*
    Defines the types of unicode encodings supported for a string in the SOME/IP message.

    Allowed range or values: [utf-8], utf-16be, utf-16le
*/
enum class serdes_string_encoding: size_t {
    utf8 = 0,
    utf16be,
    utf16le
};

constexpr serdes_string_encoding SERDES_STRING_ENCODING = serdes_string_encoding::utf8;

} // namespace threesomeip::ipc::serdes

#endif // _SERDES_CONFIGURATION_HPP