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
#include <comm_ipc.hpp>
#include <serdes/serialization.hpp>
#include <serdes/someip_types.hpp>

/*============*\
 * GOOGLETEST *
\*============*/
#include <gtest/gtest.h>


namespace {

} // anonymous namespace


namespace threesomeip_test {

using namespace threesomeip::someip::serdes;
using Buffer = std::array<std::byte, threesomeip::ipc::MAX_PAYLOAD_SIZE>;

TEST(Serdes, IntegerRoundTrip) {
    {
        Buffer buffer{};
        types::uint8 integer{static_cast<types::uint8>(0xDE)};
        serialize(buffer.data(), integer);
        ASSERT_EQ(integer, deserialize<types::uint8>(buffer.data()));
    }
    {
        Buffer buffer{};
        types::uint16 integer{static_cast<types::uint16>(0xDEAD)};
        serialize(buffer.data(), integer);
        ASSERT_EQ(integer, deserialize<types::uint16>(buffer.data()));
    }
    {
        Buffer buffer{};
        types::uint32 integer{static_cast<types::uint32>(0xDEADBEEF)};
        serialize(buffer.data(), integer);
        ASSERT_EQ(integer, deserialize<types::uint32>(buffer.data()));
    }
    {
        Buffer buffer{};
        types::uint64 integer{static_cast<types::uint64>(0xDEADBEEFBABADEDA)};
        serialize(buffer.data(), integer);
        ASSERT_EQ(integer, deserialize<types::uint64>(buffer.data()));
    }
    {
        Buffer buffer{};
        types::sint8 integer{static_cast<types::sint8>(0xDE)};
        serialize(buffer.data(), integer);
        ASSERT_EQ(integer, deserialize<types::sint8>(buffer.data()));
    }
    {
        Buffer buffer{};
        types::sint16 integer{static_cast<types::sint16>(0xDEAD)};
        serialize(buffer.data(), integer);
        ASSERT_EQ(integer, deserialize<types::sint16>(buffer.data()));
    }
    {
        Buffer buffer{};
        types::sint32 integer{static_cast<types::sint32>(0xDEADBEEF)};
        serialize(buffer.data(), integer);
        ASSERT_EQ(integer, deserialize<types::sint32>(buffer.data()));
    }
    {
        Buffer buffer{};
        types::sint64 integer{static_cast<types::sint64>(0xDEADBEEFBABADEDA)};
        serialize(buffer.data(), integer);
        ASSERT_EQ(integer, deserialize<types::sint64>(buffer.data()));
    }
}

TEST(Serdes, FloatingPointRoundTrip) {
    {
        Buffer buffer{};
        types::float32 decimal{1.2};
        serialize(buffer.data(), decimal);
        ASSERT_EQ(decimal, deserialize<types::float32>(buffer.data()));
    }
    {
        Buffer buffer{};
        types::float64 decimal{1.234};
        serialize(buffer.data(), decimal);
        ASSERT_EQ(decimal, deserialize<types::float64>(buffer.data()));
    }
}

enum class test_enum: types::uint8 {
    YES = 0,
    NO
};
TEST(Serdes, EnumRoundTrip) {
    {
        Buffer buffer{};
        test_enum enu{test_enum::YES};
        serialize(buffer.data(), enu);
        ASSERT_EQ(enu, deserialize<test_enum>(buffer.data()));
    }
}

TEST(Serdes, StringRoundTrip) {
    {
        Buffer buffer{};
        types::dlstring_utf8 string{"threesomeip"};
        serialize(buffer.data(), string);
        ASSERT_EQ(string, deserialize<types::dlstring_utf8>(buffer.data()));
    }
    {
        Buffer buffer{};
        types::flstring_utf8<12> string{'t', 'h', 'r', 'e', 'e', 's', 'o', 'm', 'e', 'i', 'p', '\0'};
        serialize(buffer.data(), string);
        ASSERT_EQ(string, deserialize<types::flstring_utf8<12>>(buffer.data()));
    }
}


struct simple_aggregate_t {
    types::uint32 field1;
    types::float32 field2;
};
bool operator==(const simple_aggregate_t& a, const simple_aggregate_t& b) {
    return a.field1 == b.field1 && a.field2 == b.field2;
}

TEST(Serdes, ArrayRoundTrip) {
    {
        Buffer buffer{};
        types::dlarray<types::uint32> array{0x00, 0x01, 0x02, 0x03};
        serialize(buffer.data(), array);
        auto read_array = deserialize<types::dlarray<types::uint32>>(buffer.data());
        ASSERT_TRUE(std::ranges::equal(array, read_array));
    }
    {
        Buffer buffer{};
        types::flarray<types::uint32, 4> array{0x00, 0x01, 0x02, 0x03};
        serialize(buffer.data(), array);
        auto read_array = deserialize<types::flarray<types::uint32, 4>>(buffer.data());
        ASSERT_TRUE(std::ranges::equal(array, read_array));
    }
    {
        Buffer buffer{};
        types::flarray<types::flarray<types::uint32, 6>, 6> array{
            types::flarray<types::uint32, 6>{0x00, 0x01, 0x02, 0x03, 0x04, 0x05},
            types::flarray<types::uint32, 6>{0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B},
            types::flarray<types::uint32, 6>{0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11},
            types::flarray<types::uint32, 6>{0x12, 0x13, 0x14, 0x15, 0x16, 0x17},
            types::flarray<types::uint32, 6>{0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D},
            types::flarray<types::uint32, 6>{0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23},
        };
        serialize(buffer.data(), array);
        auto read_array = deserialize<types::flarray<types::flarray<types::uint32, 6>, 6>>(buffer.data());
        ASSERT_TRUE(std::ranges::equal(array, read_array));
    }
    {
        Buffer buffer{};
        types::dlarray<simple_aggregate_t> array{
            {
                .field1{0x01},
                .field2{2.3}
            },
            {
                .field1{0x45},
                .field2{6.7}
            },
        };
        serialize(buffer.data(), array);
        auto read_array = deserialize<types::dlarray<simple_aggregate_t>>(buffer.data());
        ASSERT_TRUE(std::ranges::equal(array, read_array));
    }
}

struct complex_aggregate_t {
    simple_aggregate_t field1;
    types::dlarray<simple_aggregate_t> field2;
};
bool operator==(const complex_aggregate_t& a, const complex_aggregate_t& b) {
    return a.field1 == b.field1 && a.field2 == b.field2;
}

TEST(Serdes, AggregateRoundTrip) {
    {
        Buffer buffer{};
        simple_aggregate_t aggregate{
            .field1{0x01},
            .field2{2.3}
        };
        serialize(buffer.data(), aggregate);
        auto read_aggregate = deserialize<simple_aggregate_t>(buffer.data());
        ASSERT_EQ(aggregate, read_aggregate);
    }
    {
        Buffer buffer{};
        complex_aggregate_t aggregate{
            .field1{
                .field1{0x01},
                .field2{2.3}
            },
            .field2{
                {
                    .field1{0x45},
                    .field2{6.7}
                },
                {
                    .field1{0x89},
                    .field2{10.0}
                }
            }
        };
        serialize(buffer.data(), aggregate);
        auto read_aggregate = deserialize<complex_aggregate_t>(buffer.data());
        ASSERT_EQ(aggregate, read_aggregate);
    }
}

} // namespace threesomeip_test