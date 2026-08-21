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
#include <serdes/serdes_declarations.hpp>


namespace threesomeip::someip::serdes {
using namespace threesomeip::someip;


template<Array T>
void _serialize(std::byte*& out, const T& arr) {
    size_t total_length{0};
    for (const auto& element: arr) {
        total_length += _serialize_dry_run(element);
    }

    if constexpr (traits::is_dynamic_array_v<T>) {
        /* length must exist, represents the amount of bytes in the array */
        traits::dlarray_length_field_t<T> length_bytes{
            convert_endianness<config::WIRE_BYTE_ORDER>(static_cast<traits::dlarray_length_field_t<T>>(total_length))
        };
        std::memcpy(out, &length_bytes, sizeof(traits::dlarray_length_field_t<T>));
        out += sizeof(traits::dlarray_length_field_t<T>);
    }
    else if constexpr (traits::is_fixed_array_v<T>) {
        /* length is optional */
        if constexpr (! std::is_same_v<traits::flarray_length_field_t<T>, void>) {
            traits::flarray_length_field_t<T> length_bytes{
                convert_endianness<config::WIRE_BYTE_ORDER>(static_cast<traits::flarray_length_field_t<T>>(total_length))
            };
            std::memcpy(out, &length_bytes, sizeof(traits::flarray_length_field_t<T>));
            out += sizeof(traits::flarray_length_field_t<T>);
        }
    }

    for (const auto element: arr) {
        _serialize(out, element);
    }
}

template<Array T>
T _deserialize(std::byte*& in) {

    if constexpr (traits::is_dynamic_array_v<T>) {
        traits::dlarray_length_field_t<T> length{0};
        std::memcpy(&length, in, sizeof(traits::dlarray_length_field_t<T>));
        in += sizeof(traits::dlarray_length_field_t<T>);

        T ret{};

        if constexpr (requires { _serialize_dry_run<traits::array_value_t<T>>(); } ) {
            size_t length_elements{convert_endianness<config::WIRE_BYTE_ORDER>(length) / _serialize_dry_run<traits::array_value_t<T>>()};
            for (size_t i{0}; i < length_elements; ++i) {
                ret.push_back(_deserialize<traits::array_value_t<T>>(in));
            }
        }
        else {
            size_t length_remaining{convert_endianness<config::WIRE_BYTE_ORDER>(length)};
            while (length_remaining > 0) {
                auto start = in;
                ret.push_back(_deserialize<traits::array_value_t<T>>(in));
                length_remaining -= static_cast<size_t>(in - start);
            }
        }

        return ret;
    }
    else if constexpr (traits::is_fixed_array_v<T>) {
        T ret{};

        if constexpr (! std::is_same_v<traits::flarray_length_field_t<T>, void>) {
            traits::flarray_length_field_t<T> wire_length{0};
            std::memcpy(&wire_length, in, sizeof(traits::flarray_length_field_t<T>));
            in += sizeof(traits::flarray_length_field_t<T>);

            if constexpr (requires { _serialize_dry_run<traits::array_value_t<T>>(); }) {
                size_t length_elements{static_cast<size_t>(convert_endianness<config::WIRE_BYTE_ORDER>(wire_length) / _serialize_dry_run<traits::array_value_t<T>>())};
                for (size_t i{0}; i < std::min(std::tuple_size_v<T>, length_elements); ++i) {
                    ret[i] = _deserialize<traits::array_value_t<T>>(in);
                }
            }
            else {
                size_t length_remaining{static_cast<size_t>(convert_endianness<config::WIRE_BYTE_ORDER>(wire_length))};
                size_t ret_index{0};
                while (length_remaining > 0 && ret_index < std::tuple_size_v<T>) {
                    auto start = in;
                    ret[ret_index++] = _deserialize<traits::array_value_t<T>>(in);
                    length_remaining -= static_cast<size_t>(in - start);
                }
            }
        }
        else {
            for (size_t i{0}; i < std::tuple_size_v<T>; ++i) {
                ret[i] = _deserialize<traits::array_value_t<T>>(in);
            }
        }

        return ret;
    }
    else {
        static_assert(traits::always_false<T>::value, "Invalid array type");
    }

}

template<Array T>
requires traits::is_fixed_array_v<T> && requires { _serialize_dry_run<traits::array_value_t<T>>(); }
consteval size_t _serialize_dry_run() {
    constexpr size_t element_size{_serialize_dry_run<traits::array_value_t<T>>()};

    if constexpr (std::is_same_v<traits::flarray_length_field_t<T>, void>) {
        return std::tuple_size_v<T> * element_size;
    }
    else {
        return sizeof(traits::flarray_length_field_t<T>) + std::tuple_size_v<T> * element_size;
    }
}

template<Array T>
requires traits::is_fixed_array_v<T>
size_t _serialize_dry_run(const T& arr) {
    size_t elements_size{0};

    for (const auto& element: arr) {
        elements_size += _serialize_dry_run(element);
    }

    if constexpr (std::is_same_v<traits::flarray_length_field_t<T>, void>) {
        return elements_size;
    }
    else {
        return sizeof(traits::flarray_length_field_t<T>) + elements_size;
    }
}

template<Array T>
requires traits::is_dynamic_array_v<T>
size_t _serialize_dry_run(const T& arr) {
    size_t elements_size{0};

    for (const auto& element: arr) {
        elements_size += _serialize_dry_run(element);
    }

    if constexpr (std::is_same_v<traits::dlarray_length_field_t<T>, void>) {
        return elements_size;
    }
    else {
        return sizeof(traits::dlarray_length_field_t<T>) + elements_size;
    }
}

} // namespace threesomeip::someip::serdes

#endif // _ARRAYS_HPP