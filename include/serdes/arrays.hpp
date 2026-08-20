#ifndef _ARRAYS_HPP
#define _ARRAYS_HPP

/*=====*\
 * C++ *
\*=====*/
#include <type_traits>
#include <bit>
#include <cstdint>
#include <cstddef>
#include <span>
#include <cstring>
#include <algorithm>

/*=============*\
 * APPLICATION *
\*=============*/
#include <serdes/concepts.hpp>
#include <serdes/serdes_configuration.hpp>
#include <serdes/someip_types.hpp>
#include <serdes/someip_type_traits.hpp>
#include <serdes/integers.hpp>

namespace threesomeip::someip::serdes {
using namespace threesomeip::someip;


template<Array T>
using array_value_t = std::ranges::range_value_t<T>;

using flarray_length_field_t =
    std::conditional_t<config::SIZE_OF_FIXED_ARRAY_LENGTH_FIELD == 1, uint8_t,
        std::conditional_t<config::SIZE_OF_FIXED_ARRAY_LENGTH_FIELD == 2, uint16_t,
            std::conditional_t<config::SIZE_OF_FIXED_ARRAY_LENGTH_FIELD == 4, uint32_t,
               void
            >
        >
    >;

using dlarray_length_field_t =
    std::conditional_t<config::SIZE_OF_DYNAMIC_ARRAY_LENGTH_FIELD == 1, uint8_t,
        std::conditional_t<config::SIZE_OF_DYNAMIC_ARRAY_LENGTH_FIELD == 2, uint16_t,
            std::conditional_t<config::SIZE_OF_DYNAMIC_ARRAY_LENGTH_FIELD == 4, uint32_t,
               /* prohibited from being 0 by configuration validator*/ void
            >
        >
    >;


/* 
    HERE THE sizeof(array_value_t<T>) IS NOT APPROPRIATE BECAUSE SOME TYPES MAY NOT MAP 1:1 SERIALIZED AND IN-MEMORY;REWORK
*/

template<Array T>
void _serialize(std::byte*& out, const T& arr) {
    if constexpr (traits::is_dynamic_array_v<T>) {
        /* length must exist, represents the amount of bytes in the array */
        dlarray_length_field_t length_bytes{
            convert_endianness<config::WIRE_BYTE_ORDER>(static_cast<dlarray_length_field_t>(arr.size() * sizeof(array_value_t<T>)))
        };
        std::memcpy(out, &length_bytes, sizeof(dlarray_length_field_t));
        out += sizeof(dlarray_length_field_t);
    }
    else if constexpr (traits::is_fixed_array_v<T>) {
        /* length is optional */
        if constexpr (! std::is_same_v<flarray_length_field_t, void>) {
            constexpr flarray_length_field_t length_bytes{convert_endianness<config::WIRE_BYTE_ORDER>(
                static_cast<flarray_length_field_t>(std::tuple_size_v<T> * sizeof(array_value_t<T>))
            )};
            std::memcpy(out, &length_bytes, sizeof(flarray_length_field_t));
            out += sizeof(flarray_length_field_t);
        }
    }

    for (const auto element: arr) {
        _serialize(out, element);
    }
}

template<Array T>
T _deserialize(std::byte*& in) {

    if constexpr (traits::is_dynamic_array_v<T>) {
        dlarray_length_field_t length{0};
        std::memcpy(&length, in, sizeof(dlarray_length_field_t));
        in += sizeof(dlarray_length_field_t);
        size_t length_elements{convert_endianness<config::WIRE_BYTE_ORDER>(length) / sizeof(array_value_t<T>)};

        T ret{};

        for (size_t i{0}; i < length_elements; ++i) {
            ret.push_back(_deserialize<array_value_t<T>>(in));
        }

        return ret;
    }
    else if constexpr (traits::is_fixed_array_v<T>) {
        size_t length_elements{0};

        if constexpr (! std::is_same_v<flarray_length_field_t, void>) {
            flarray_length_field_t wire_length{0};
            std::memcpy(&wire_length, in, sizeof(flarray_length_field_t));
            in += sizeof(flarray_length_field_t);
            length_elements = static_cast<size_t>(convert_endianness<config::WIRE_BYTE_ORDER>(wire_length) / sizeof(array_value_t<T>));
        }
        else {
            length_elements = std::tuple_size_v<T>;
        }

        T ret{};

        for (size_t i{0}; i < std::min(length_elements, std::tuple_size_v<T>); ++i) {
            ret[i] = _deserialize<array_value_t<T>>(in);
        }

        return ret;
    }
    else {
        static_assert(traits::always_false<T>::value, "Invalid array type");
    }

}

template<Array T>
requires traits::is_fixed_array_v<T> && requires { _serialize_dry_run<array_value_t<T>>(); }
consteval size_t _serialize_dry_run() {
    constexpr size_t element_size{_serialize_dry_run<array_value_t<T>>()};

    if constexpr (std::is_same_v<flarray_length_field_t, void>) {
        return std::tuple_size_v<T> * element_size;
    }
    else {
        return sizeof(flarray_length_field_t) + std::tuple_size_v<T> * element_size;
    }
}

template<Array T>
requires traits::is_fixed_array_v<T>
size_t _serialize_dry_run(const T& arr) {
    size_t elements_size{0};

    for (const auto& element: arr) {
        elements_size += _serialize_dry_run(element);
    }

    if constexpr (std::is_same_v<flarray_length_field_t, void>) {
        return elements_size;
    }
    else {
        return sizeof(flarray_length_field_t) + elements_size;
    }
}

template<Array T>
requires traits::is_dynamic_array_v<T>
size_t _serialize_dry_run(const T& arr) {
    size_t elements_size{0};

    for (const auto& element: arr) {
        elements_size += _serialize_dry_run(element);
    }

    if constexpr (std::is_same_v<dlarray_length_field_t, void>) {
        return elements_size;
    }
    else {
        return sizeof(dlarray_length_field_t) + elements_size;
    }
}

} // namespace threesomeip::someip::serdes

#endif // _ARRAYS_HPP