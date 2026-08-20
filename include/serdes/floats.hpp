#ifndef _FLOATS_HPP
#define _FLOATS_HPP

/*=====*\
 * C++ *
\*=====*/
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>

/*=============*\
 * APPLICATION *
\*=============*/
#include <serdes/concepts.hpp>
#include <serdes/someip_type_traits.hpp>
#include <serdes/serdes_configuration.hpp>


namespace threesomeip::someip::serdes {

template<std::endian TargetEndianness, FloatingPoint T>
constexpr T convert_endianness(T num) {
    if constexpr (std::endian::native != TargetEndianness) {
        if constexpr (sizeof(T) == sizeof(uint32_t)) {
            auto tmp = std::byteswap(std::bit_cast<uint32_t>(num));
            return std::bit_cast<T>(tmp);
        }
        else if constexpr (sizeof(T) == sizeof(uint64_t)) {
            auto tmp = std::byteswap(std::bit_cast<uint64_t>(num));
            return std::bit_cast<T>(tmp);
        }
        else {
            static_assert(traits::always_false<T>::value, "Invalid type sizes for target platform.");
        }
    }
    else {
        return num;
    }
}

template<FloatingPoint T>
void _serialize(std::byte*& out, const T& num) {
    const T _num{convert_endianness<config::WIRE_BYTE_ORDER>(num)};
    std::memcpy(out, &_num, sizeof(T));
    out += sizeof(T);
}

template<FloatingPoint T>
T _deserialize(std::byte*& in) {
    T num{0};
    std::memcpy(&num, in, sizeof(T));
    in += sizeof(T);
    return convert_endianness<config::WIRE_BYTE_ORDER>(num);
}

template<FloatingPoint T>
consteval size_t _serialize_dry_run() {
    return sizeof(T);
}

template<FloatingPoint T>
constexpr size_t _serialize_dry_run([[maybe_unused]] const T& num) {
    return sizeof(T);
}

} // namespace threesomeip::someip::serdes

#endif // _FLOATS_HPP