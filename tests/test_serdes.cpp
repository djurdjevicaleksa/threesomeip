/*=====*\
 * C++ *
\*=====*/
#include <cstdint>
#include <cstddef>
#include <array>
#include <vector>
#include <span>
#include <string>
#include <algorithm>
#include <iostream>

/*=============*\
 * APPLICATION *
\*=============*/
#include <serialization.hpp>
#include <ipc_format.hpp>

/*============*\
 * GOOGLETEST *
\*============*/
#include <gtest/gtest.h>


namespace {

} // anonymous namespace


namespace threesomeip_test {

using namespace threesomeip::ipc::serdes;
using Buffer = std::array<std::byte, threesomeip::ipc::MAX_PAYLOAD_SIZE>;

TEST(Serdes, StringRoundTrip) {
    {
        Buffer buffer{};
        std::string string{"threesomeip"};
        serialize(buffer.data(), string);
        ASSERT_EQ(string, deserialize<std::string>(buffer.data()));
    }
    {
        Buffer buffer{};
        std::string string{"threesomeip"};
        serialize(buffer.data(), std::string_view(string));
        ASSERT_EQ(string, deserialize<std::string_view>(buffer.data()));
    }
}

TEST(Serdes, UnsignedIntegerRoundTrip) {
    {
        Buffer buffer{};
        uint16_t integer{0xDEAD};
        serialize(buffer.data(), integer);
        ASSERT_EQ(integer, deserialize<uint16_t>(buffer.data()));
    }
    {
        Buffer buffer{};
        uint32_t integer{0xDEADBEEF};
        serialize(buffer.data(), integer);
        ASSERT_EQ(integer, deserialize<uint32_t>(buffer.data()));
    }
    {
        Buffer buffer{};
        uint64_t integer{0xDEADBEEFBABADEDA};
        serialize(buffer.data(), integer);
        ASSERT_EQ(integer, deserialize<uint64_t>(buffer.data()));
    }
}

TEST(Serdes, ByteRoundTrip) {
    {
        Buffer buffer{};
        uint8_t byte{0xAB};
        serialize(buffer.data(), byte);
        ASSERT_EQ(byte, deserialize<uint8_t>(buffer.data()));
    }
    {
        Buffer buffer{};
        std::byte byte{0xCD};
        serialize(buffer.data(), byte);
        ASSERT_EQ(byte, deserialize<std::byte>(buffer.data()));
    }
}

TEST(Serdes, ArrayRoundTrip) {
    {
        Buffer buffer{};
        std::array<uint16_t, 3> array{1, 2, 3};
        serialize(buffer.data(), array);
        auto read_array = deserialize<std::span<const uint16_t>>(buffer.data());
        ASSERT_TRUE(std::ranges::equal(array, read_array));
    }
    {
        Buffer buffer{};
        std::array<uint16_t, 3> array{1, 2, 3};
        serialize(buffer.data(), array);
        auto read_array = deserialize<std::vector<uint16_t>>(buffer.data());
        ASSERT_TRUE(std::ranges::equal(array, read_array));
    }
    {
        Buffer buffer{};
        std::vector<uint16_t> array{1, 2, 3};
        serialize(buffer.data(), array);
        auto read_array = deserialize<std::span<const uint16_t>>(buffer.data());
        ASSERT_TRUE(std::ranges::equal(array, read_array));
    }
    {
        Buffer buffer{};
        std::vector<uint16_t> array{1, 2, 3};
        serialize(buffer.data(), array);
        auto read_array = deserialize<std::vector<uint16_t>>(buffer.data());
        ASSERT_TRUE(std::ranges::equal(array, read_array));
    }
    {
        Buffer buffer{};
        uint16_t array[]{1, 2, 3};
        serialize(buffer.data(), array);
        auto read_array = deserialize<std::span<const uint16_t>>(buffer.data());
        ASSERT_TRUE(std::ranges::equal(array, read_array));
    }
    {
        Buffer buffer{};
        uint16_t array[]{1, 2, 3};
        serialize(buffer.data(), array);
        auto read_array = deserialize<std::vector<uint16_t>>(buffer.data());
        ASSERT_TRUE(std::ranges::equal(array, read_array));
    }
}


namespace {

struct simple_aggregate_t {
    std::byte field1;
    uint16_t field2;
    std::string field3;
    std::array<uint32_t, 5> field4;
};

struct complex_aggregate_t {
    simple_aggregate_t field1;
    uint32_t field2;
    simple_aggregate_t field3;
};

bool operator==(const simple_aggregate_t& a, const simple_aggregate_t& b) {
    if (a.field1 != b.field1) return false;
    if (a.field2 != b.field2) return false;
    if (a.field3 != b.field3) return false;
    if (!std::ranges::equal(a.field4, b.field4)) return false;
    return true;
}

bool operator==(const complex_aggregate_t& a, const complex_aggregate_t& b) {
    if (a.field1 != b.field1) return false;
    if (a.field2 != b.field2) return false;
    if (a.field3 != b.field3) return false;
    return true;
}

} // anonymous namespace

TEST(Serdes, AggregateRoundTrip) {
    {
        Buffer buffer{};
        simple_aggregate_t simple_aggregate{
            .field1{static_cast<std::byte>(1)},
            .field2{static_cast<uint16_t>(2)},
            .field3{"threesomeip"},
            .field4{3, 4, 5, 6, 7}
        };
        serialize(buffer.data(), simple_aggregate);
        auto read_simple_aggregate = deserialize<simple_aggregate_t>(buffer.data());

        ASSERT_EQ(simple_aggregate, read_simple_aggregate);
    }
    {
        Buffer buffer{};
        complex_aggregate_t complex_aggregate{
            .field1{
                .field1{static_cast<std::byte>(1)},
                .field2{static_cast<uint16_t>(2)},
                .field3{"threesomeip"},
                .field4{3, 4, 5, 6, 7}
            },
            .field2{8},
            .field3{
                .field1{static_cast<std::byte>(9)},
                .field2{static_cast<uint16_t>(10)},
                .field3{"autosar"},
                .field4{11, 12, 13, 14, 15}
            }
        };
        serialize(buffer.data(), complex_aggregate);
        auto read_complex_aggregate = deserialize<complex_aggregate_t>(buffer.data());

        ASSERT_EQ(complex_aggregate, read_complex_aggregate);
    }
}

} // namespace threesomeip_test