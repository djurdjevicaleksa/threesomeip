#ifndef _STRINGS_HPP
#define _STRINGS_HPP

/*=====*\
 * C++ *
\*=====*/
#include <type_traits>
#include <cstdint>
#include <cstddef>
#include <bit>
#include <cstring>
#include <ranges>
#include <algorithm>

/*=============*\
 * APPLICATION *
\*=============*/
#include <serdes/someip_types.hpp>
#include <serdes/someip_type_traits.hpp>
#include <serdes/concepts.hpp>
#include <serdes/integers.hpp>
#include <serdes/serdes_configuration.hpp>


namespace threesomeip::someip::serdes {
using namespace threesomeip::someip;


inline constexpr std::array<std::byte, 3> bom_utf8{std::byte{0xEF}, std::byte{0xBB}, std::byte{0xBF}};

template<UTF8String T>
void _serialize(std::byte*& out, const T& str) {
    if constexpr (traits::is_dynamic_string_v<T>) {
        /* length must exist, contain BOM, data, and null terminator and must be big endian */
        traits::dlstring_length_field_t<T> length{convert_endianness<config::WIRE_BYTE_ORDER>(
            static_cast<traits::dlstring_length_field_t<T>>(bom_utf8.size() + str.size() /* '\0' */ + size_t{1})
        )};
        std::memcpy(out, &length, sizeof(traits::dlstring_length_field_t<T>));
        out += sizeof(traits::dlstring_length_field_t<T>);
    }
    else if constexpr (traits::is_fixed_string_v<T>) {
        /* length optional, contains BOM, data, and null terminator and must be big endian */
        if constexpr (! std::is_same_v<traits::flstring_length_field_t<T>, void>) {
            constexpr traits::flstring_length_field_t<T> length{convert_endianness<config::WIRE_BYTE_ORDER>(
                static_cast<traits::flstring_length_field_t<T>>(bom_utf8.size() + std::tuple_size_v<T> /* '\0' */ + size_t{1})
            )};
            std::memcpy(out, &length, sizeof(traits::flstring_length_field_t<T>));
            out += sizeof(traits::flstring_length_field_t<T>);
        }
    }
    else {
        static_assert(traits::always_false<T>::value, "Invalid utf8string type");
    }

    std::memcpy(out, bom_utf8.data(), bom_utf8.size());
    out += bom_utf8.size();

    std::memcpy(out, reinterpret_cast<const std::byte*>(str.data()), str.size());
    out += str.size();

    *out = static_cast<std::byte>('\0');
    ++out;
}

template<UTF8String T>
T _deserialize(std::byte*& in) {
    if constexpr (traits::is_dynamic_string_v<T>) {
        /* get length; move cursor */
        traits::dlstring_length_field_t<T> length{0};
        std::memcpy(&length, in, sizeof(traits::dlstring_length_field_t<T>));
        in += sizeof(traits::dlstring_length_field_t<T>);
        length = convert_endianness<config::WIRE_BYTE_ORDER>(length);

        /* check bom */
        if (! std::ranges::equal(
            std::span<const std::byte>(in, bom_utf8.size()),
            bom_utf8
        )) {
            /* skip bom + text, return blank string */
            in += length;
            return T{};
        }
        /* bom ok, move cursor */
        in += bom_utf8.size();

        /* construct string, move cursor */
        T ret{reinterpret_cast<const char*>(in), length - bom_utf8.size() /* '\0' */ - size_t{1}};
        in += length - bom_utf8.size(); /* skip '\0' which was added during serialization */
        return ret;
    }
    else if constexpr (traits::is_fixed_string_v<T>) {
        size_t length{0};

        if constexpr (! std::is_same_v<traits::flstring_length_field_t<T>, void>) {
            traits::flstring_length_field_t<T> wire_length{0};
            std::memcpy(&wire_length, in, sizeof(traits::flstring_length_field_t<T>));
            in += sizeof(traits::flstring_length_field_t<T>);
            length = static_cast<size_t>(convert_endianness<config::WIRE_BYTE_ORDER>(wire_length));
        }
        else {
            length = bom_utf8.size() + std::tuple_size_v<T> /* '\0' */ + size_t{1};
        }


        /* check bom */
        if (! std::ranges::equal(
            std::span<const std::byte>(in, bom_utf8.size()),
            bom_utf8
        )) {
            /* skip bom + text, return blank string */
            in += length;
            return T{};
        }
        /* bom ok, move cursor */
        in += bom_utf8.size();

        T ret{};

        /* copy data, move cursor */
        std::ranges::copy_n(
            reinterpret_cast<const char*>(in),
            std::min(static_cast<size_t>(length - bom_utf8.size() /* '\0' */ - size_t{1}), std::tuple_size_v<T>),
            ret.begin()
        );
        in += length - bom_utf8.size(); /* skip '\0' which was added during serialization */
        return ret;
    }
    else {
        static_assert(traits::always_false<T>::value, "Invalid utf8string type");
    }
}

template<UTF8String T>
requires traits::is_fixed_string_v<T>
consteval size_t _serialize_dry_run() {
    if constexpr (std::is_same_v<traits::flstring_length_field_t<T>, void>) {
        return bom_utf8.size() + std::tuple_size_v<T> /* '\0' */ + size_t{1};
    }
    else {
        return sizeof(traits::flstring_length_field_t<T>) + bom_utf8.size() + std::tuple_size_v<T> /* '\0' */ + size_t{1};
    }
}

template<UTF8String T>
requires traits::is_dynamic_string_v<T>
size_t _serialize_dry_run(const T& str) {
    return sizeof(traits::dlstring_length_field_t<T>) + bom_utf8.size() + str.size() /* '\0' */ + size_t{1};
}

} // namespace threesomeip::someip::serdes

#endif // _STRINGS_HPP