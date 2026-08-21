#ifndef _SERDES_DECLARATIONS_HPP
#define _SERDES_DECLARATIONS_HPP

/*=====*\
 * C++ *
\*=====*/
#include <cstddef>

/*=============*\
 * APPLICATION *
\*=============*/
#include <serdes/concepts.hpp>
#include <serdes/someip_type_traits.hpp>


namespace threesomeip::someip::serdes {


template<Integer T>
void _serialize(std::byte*& out, const T& num);

template<Integer T>
T _deserialize(std::byte*& in);

template<Integer T>
consteval size_t _serialize_dry_run();

template<Integer T>
constexpr size_t _serialize_dry_run([[maybe_unused]] const T& num);


template<FloatingPoint T>
void _serialize(std::byte*& out, const T& num);

template<FloatingPoint T>
T _deserialize(std::byte*& in);

template<FloatingPoint T>
consteval size_t _serialize_dry_run();

template<FloatingPoint T>
constexpr size_t _serialize_dry_run([[maybe_unused]] const T& num);


template<Enum T>
void _serialize(std::byte*& out, const T& enu);

template<Enum T>
T _deserialize(std::byte*& in);

template<Enum T>
consteval size_t _serialize_dry_run();

template<Enum T>
constexpr size_t _serialize_dry_run([[maybe_unused]] const T& enu);


template<UTF8String T>
void _serialize(std::byte*& out, const T& str);

template<UTF8String T>
T _deserialize(std::byte*& in);

template<UTF8String T>
requires traits::is_fixed_string_v<T>
consteval size_t _serialize_dry_run();

template<UTF8String T>
requires traits::is_dynamic_string_v<T>
size_t _serialize_dry_run(const T& str);


template<Array T>
void _serialize(std::byte*& out, const T& arr);

template<Array T>
T _deserialize(std::byte*& in);

template<Array T>
requires traits::is_fixed_array_v<T>
size_t _serialize_dry_run(const T& arr);

template<Array T>
requires traits::is_dynamic_array_v<T>
size_t _serialize_dry_run(const T& arr);


template<Aggregate T>
void _serialize(std::byte*& out, const T& agg);

template<Aggregate T>
T _deserialize(std::byte*& in);

template<Aggregate T>
size_t _serialize_dry_run(const T& aggregate);

} // namespace threesomeip::someip::serdes



#endif // _SERDES_DECLARATIONS_HPP