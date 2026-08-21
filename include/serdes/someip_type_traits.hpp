#ifndef _SOMEIP_TYPE_TRAITS_HPP
#define _SOMEIP_TYPE_TRAITS_HPP

/*=====*\
 * C++ *
\*=====*/
#include <type_traits>
#include <vector>
#include <array>
#include <cstdint>

/*=============*\
 * APPLICATION *
\*=============*/
#include <serdes/someip_types.hpp>
#include <serdes/serdes_configuration.hpp>


namespace threesomeip::someip::traits {
using namespace threesomeip::someip;

/*
    Utilities
*/
template<typename T>
struct always_false : std::false_type {};

template<typename Excluded>
struct any_except {
    template<typename T>
    requires (!std::same_as<T, Excluded>)
    constexpr operator T() const noexcept;
};


/*
    Strings
*/
template<typename T>
struct is_fixed_string: std::false_type {};

template<size_t N>
struct is_fixed_string<types::flstring_utf8<N>>: std::true_type {};

template<typename T>
inline constexpr bool is_fixed_string_v = is_fixed_string<std::remove_cvref_t<T>>::value;

template<typename T>
inline constexpr bool is_dynamic_string_v = std::is_same_v<std::remove_cvref_t<T>, types::dlstring_utf8>;

template<typename T>
struct flstring_length_field {
    using type =
        std::conditional_t<config::SIZE_OF_FIXED_STRING_LENGTH_FIELD == 1, uint8_t,
            std::conditional_t<config::SIZE_OF_FIXED_STRING_LENGTH_FIELD == 2, uint16_t,
                std::conditional_t<config::SIZE_OF_FIXED_STRING_LENGTH_FIELD == 4, uint32_t,
                void
                >
            >
        >;
};

template<typename T>
using flstring_length_field_t = flstring_length_field<T>::type;

template<typename T>
struct dlstring_length_field {
    using type =
        std::conditional_t<config::SIZE_OF_DYNAMIC_STRING_LENGTH_FIELD == 1, uint8_t,
            std::conditional_t<config::SIZE_OF_DYNAMIC_STRING_LENGTH_FIELD == 2, uint16_t,
                std::conditional_t<config::SIZE_OF_DYNAMIC_STRING_LENGTH_FIELD == 4, uint32_t,
                /* prohibited from being 0 by configuration validator*/ void
                >
            >
        >;
};

template<typename T>
using dlstring_length_field_t = dlstring_length_field<T>::type;

/*
    Arrays
*/
template<typename T>
struct is_fixed_array: std::false_type {};

template<typename T, size_t N>
struct is_fixed_array<types::flarray<T, N>>: std::true_type {};

template<typename T>
inline constexpr bool is_fixed_array_v = is_fixed_array<std::remove_cvref_t<T>>::value;

template<typename T>
struct is_dynamic_array: std::false_type {};

template<typename T>
struct is_dynamic_array<types::dlarray<T>>: std::true_type {};

template<typename T>
inline constexpr bool is_dynamic_array_v = is_dynamic_array<std::remove_cvref_t<T>>::value;

template<typename T>
struct flarray_length_field {
    using type =
        std::conditional_t<config::SIZE_OF_FIXED_ARRAY_LENGTH_FIELD == 1, uint8_t,
            std::conditional_t<config::SIZE_OF_FIXED_ARRAY_LENGTH_FIELD == 2, uint16_t,
                std::conditional_t<config::SIZE_OF_FIXED_ARRAY_LENGTH_FIELD == 4, uint32_t,
                void
                >
            >
        >;
};

template<typename T>
using flarray_length_field_t = flarray_length_field<T>::type;

template<typename T>
struct dlarray_length_field {
    using type =
        std::conditional_t<config::SIZE_OF_DYNAMIC_ARRAY_LENGTH_FIELD == 1, uint8_t,
            std::conditional_t<config::SIZE_OF_DYNAMIC_ARRAY_LENGTH_FIELD == 2, uint16_t,
                std::conditional_t<config::SIZE_OF_DYNAMIC_ARRAY_LENGTH_FIELD == 4, uint32_t,
                   /* prohibited from being 0 by configuration validator*/ void
                >
            >
        >;
};

template<typename T>
using dlarray_length_field_t = dlarray_length_field<T>::type;

template<typename T>
using array_value_t = std::ranges::range_value_t<T>;

/*
    Aggregates
    Note: Template param used to force two-phase template resolution
*/
template<typename T>
struct aggregate_length_field {
    using type =
        std::conditional_t<config::SIZE_OF_STRUCT_LENGTH_FIELD == 1, uint8_t,
            std::conditional_t<config::SIZE_OF_STRUCT_LENGTH_FIELD == 2, uint16_t,
                std::conditional_t<config::SIZE_OF_STRUCT_LENGTH_FIELD == 4, uint32_t,
                    void
                >
            >
        >;
};

template<typename T>
using aggregate_length_field_t = aggregate_length_field<T>::type;


} // namespace threesomeip::someip::traits

#endif // _SOMEIP_TYPE_TRAITS_HPP