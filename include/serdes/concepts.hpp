#ifndef _CONCEPTS_HPP
#define _CONCEPTS_HPP

/*=====*\
 * C++ *
\*=====*/
#include <type_traits>

/*=============*\
 * APPLICATION *
\*=============*/
#include <serdes/someip_types.hpp>
#include <serdes/someip_type_traits.hpp>


namespace threesomeip::someip::serdes {

template<typename T>
concept UnsignedInteger =
       std::is_same_v<std::remove_cvref_t<T>, types::uint8>
    || std::is_same_v<std::remove_cvref_t<T>, types::uint16>
    || std::is_same_v<std::remove_cvref_t<T>, types::uint32>
    || std::is_same_v<std::remove_cvref_t<T>, types::uint64>;

template<typename T>
concept SignedInteger =
       std::is_same_v<std::remove_cvref_t<T>, types::sint8>
    || std::is_same_v<std::remove_cvref_t<T>, types::sint16>
    || std::is_same_v<std::remove_cvref_t<T>, types::sint32>
    || std::is_same_v<std::remove_cvref_t<T>, types::sint64>;

template<typename T>
concept Integer = UnsignedInteger<T> || SignedInteger<T>;

template<typename T>
concept FloatingPoint =
       std::is_same_v<std::remove_cvref_t<T>, types::float32>
    || std::is_same_v<std::remove_cvref_t<T>, types::float64>;

template<typename T>
concept Enum =
       std::is_enum_v<std::remove_cvref_t<T>>
    && UnsignedInteger<std::underlying_type_t<std::remove_cvref_t<T>>>;

template<typename T>
concept UTF8String =
       traits::is_fixed_string_v<T>
    || traits::is_dynamic_string_v<T>;

template<typename T>
concept Array =
    (traits::is_fixed_array_v<T> || traits::is_dynamic_array_v<T>)
    && !UTF8String<T>;

template<typename T>
concept Aggregate =
       std::is_aggregate_v<std::remove_cvref_t<T>>
    && std::is_class_v<std::remove_cvref_t<T>>
    && !Array<T>
    && !UTF8String<T>;


} // namespace threesomeip::someip::serdes




#endif // _CONCEPTS_HPP