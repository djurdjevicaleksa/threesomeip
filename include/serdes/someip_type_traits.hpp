#ifndef _SOMEIP_TYPE_TRAITS_HPP
#define _SOMEIP_TYPE_TRAITS_HPP

/*=====*\
 * C++ *
\*=====*/
#include <type_traits>
#include <vector>
#include <array>

/*=============*\
 * APPLICATION *
\*=============*/
#include <serdes/someip_types.hpp>


namespace threesomeip::someip::traits {
using namespace threesomeip::someip::types;

/*
    Utilities
*/
template<typename T>
struct always_false : std::false_type {};


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
inline constexpr bool is_dynamic_string_v = std::is_same_v<std::remove_cvref_t<T>, dlstring_utf8>;

/*
    Arrays
*/
template<typename T>
struct is_fixed_array: std::false_type {};

template<typename T, size_t N>
struct is_fixed_array<flarray<T, N>>: std::true_type {};

template<typename T>
inline constexpr bool is_fixed_array_v = is_fixed_array<std::remove_cvref_t<T>>::value;

template<typename T>
struct is_dynamic_array: std::false_type {};

template<typename T>
struct is_dynamic_array<dlarray<T>>: std::true_type {};

template<typename T>
inline constexpr bool is_dynamic_array_v = is_dynamic_array<std::remove_cvref_t<T>>::value;


} // namespace threesomeip::someip::types::traits



#endif // _SOMEIP_TYPE_TRAITS_HPP