#ifndef _TEST_SERDES_TEMPLATES_HPP
#define _TEST_SERDES_TEMPLATES_HPP

/*=====*\
 * C++ *
\*=====*/
#include <cstdint>
#include <cstddef>

/*=============*\
 * APPLICATION *
\*=============*/
#include <serialization.hpp>


using namespace threesomeip::ipc::serdes;


/*
    UnsignedInteger template tests
*/
static_assert(!UnsignedInteger<uint8_t>);
static_assert(!UnsignedInteger<const uint8_t>);
static_assert(!UnsignedInteger<uint8_t&>);
static_assert(!UnsignedInteger<const uint8_t&>);

static_assert(UnsignedInteger<uint16_t>);
static_assert(UnsignedInteger<const uint16_t>);
static_assert(UnsignedInteger<uint16_t&>);
static_assert(UnsignedInteger<const uint16_t&>);

static_assert(UnsignedInteger<uint32_t>);
static_assert(UnsignedInteger<const uint32_t>);
static_assert(UnsignedInteger<uint32_t&>);
static_assert(UnsignedInteger<const uint32_t&>);

static_assert(UnsignedInteger<uint64_t>);
static_assert(UnsignedInteger<const uint64_t>);
static_assert(UnsignedInteger<uint64_t&>);
static_assert(UnsignedInteger<const uint64_t&>);

/*
    Byte template tests
*/
static_assert(Byte<uint8_t>);
static_assert(Byte<const uint8_t>);
static_assert(Byte<uint8_t&>);
static_assert(Byte<const uint8_t&>);

static_assert(Byte<std::byte>);
static_assert(Byte<const std::byte>);
static_assert(Byte<std::byte&>);
static_assert(Byte<const std::byte&>);

#endif // _TEST_SERDES_TEMPLATES_HPP