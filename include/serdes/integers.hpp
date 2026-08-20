#ifndef _INTEGERS_HPP
#define _INTEGERS_HPP

/*=====*\
 * C++ *
\*=====*/
#include <bit>
#include <cstddef>
#include <cstring>

/*=============*\
 * APPLICATION *
\*=============*/
#include <serdes/concepts.hpp>
#include <serdes/serdes_configuration.hpp>


namespace threesomeip::someip::serdes {

template<std::endian TargetEndianness, Integer T>
constexpr T convert_endianness(T num) {
    if constexpr (std::endian::native != TargetEndianness) {
        return std::byteswap(num);
    }
    else {
        return num;
    }
}

template<Integer T>
void _serialize(std::byte*& out, const T& num) {
    T _num{convert_endianness<config::WIRE_BYTE_ORDER>(num)};
    std::memcpy(out, &_num, sizeof(T));
    out += sizeof(T);
}

template<Integer T>
T _deserialize(std::byte*& in) {
    T num{0};
    std::memcpy(&num, in, sizeof(T));
    in += sizeof(T);
    return convert_endianness<config::WIRE_BYTE_ORDER>(num);
}

template<Integer T>
consteval size_t _serialize_dry_run() {
    return sizeof(T);
}

template<Integer T>
constexpr size_t _serialize_dry_run([[maybe_unused]] const T& num) {
    return sizeof(T);
}

} // namespace threesomeip::someip::types

#endif // _INTEGERS_HPP