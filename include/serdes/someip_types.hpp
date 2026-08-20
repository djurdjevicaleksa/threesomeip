#ifndef _SERDES_TYPES_HPP
#define _SERDES_TYPES_HPP

/*=====*\
 * C++ *
\*=====*/
#include <cstdint>
#include <string>
#include <array>


namespace threesomeip::someip::types {

using sbool = bool;

using sint8 = int8_t;
using sint16 = int16_t;
using sint32 = int32_t;
using sint64 = int64_t;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using float32 = float;
using float64 = double;


using dlstring_utf8 = std::string;

template<size_t N>
using flstring_utf8 = std::array<char, N>;

template<typename T>
using dlarray = std::vector<T>;

template<typename T, size_t N>
using flarray = std::array<T, N>;


} // threesomeip::someip::types


#endif // _SERDES_TYPES_HPP