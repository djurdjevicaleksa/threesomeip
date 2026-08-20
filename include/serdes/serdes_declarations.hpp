#ifndef _SERDES_DECLARATIONS_HPP
#define _SERDES_DECLARATIONS_HPP

/*=====*\
 * C++ *
\*=====*/
#include <bit>
#include <cstddef>

/*=============*\
 * APPLICATION *
\*=============*/
#include <serdes/concepts.hpp>


namespace threesomeip::someip::serdes {


template<Integer T>
void _serialize(std::byte*& out, const T& num);

template<Integer T>
T _deserialize(std::byte*& in);

template<FloatingPoint T>
void _serialize(std::byte*& out, const T& num);

template<FloatingPoint T>
T _deserialize(std::byte*& in);

template<Enum T>
void _serialize(std::byte*& out, const T& enu);

template<Enum T>
T _deserialize(std::byte*& in);

template<UTF8String T>
void _serialize(std::byte*& out, const T& str);

template<UTF8String T>
T _deserialize(std::byte*& in);

template<Array T>
void _serialize(std::byte*& out, const T& arr);

template<Array T>
T _deserialize(std::byte*& in);





} // namespace threesomeip::someip::serdes



#endif // _SERDES_DECLARATIONS_HPP