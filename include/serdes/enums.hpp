#ifndef _ENUMS_HPP
#define _ENUMS_HPP

/*=====*\
 * C++ *
\*=====*/
#include <bit>
#include <cstddef>
#include <cstring>
#include <type_traits>

/*=============*\
 * APPLICATION *
\*=============*/
#include <serdes/concepts.hpp>
#include <serdes/integers.hpp>
#include <serdes/serdes_configuration.hpp>

namespace threesomeip::someip::serdes {

template<Enum T>
void _serialize(std::byte*& out, const T& enu) {
    using actual_type = std::underlying_type_t<std::remove_cvref_t<T>>;
    const actual_type _enu{convert_endianness<config::WIRE_BYTE_ORDER>(static_cast<actual_type>(enu))};
    std::memcpy(out, &_enu, sizeof(actual_type));
    out += sizeof(actual_type);
}

template<Enum T>
T _deserialize(std::byte*& in) {
    using actual_type = std::underlying_type_t<std::remove_cvref_t<T>>;
    actual_type enu{0};
    std::memcpy(&enu, in, sizeof(actual_type));
    in += sizeof(actual_type);
    return static_cast<T>(convert_endianness<config::WIRE_BYTE_ORDER>(enu));
}

template<Enum T>
consteval size_t _serialize_dry_run() {
    return sizeof(std::underlying_type_t<std::remove_cvref_t<T>>);
}

template<Enum T>
constexpr size_t _serialize_dry_run([[maybe_unused]] const T& enu) {
    return sizeof(std::underlying_type_t<std::remove_cvref_t<T>>);
}

} // namespace threesomeip::someip::serdes

#endif // _ENUMS_HPP