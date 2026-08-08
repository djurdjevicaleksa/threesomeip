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
        ASSERT_EQ(string, std::get<0>(deserialize<std::string>(buffer.data())));
    }
    {
        Buffer buffer{};
        std::string string{"threesomeip"};
        serialize(buffer.data(), std::string_view(string));
        ASSERT_EQ(string, std::get<0>(deserialize<std::string_view>(buffer.data())));
    }
}

TEST(Serdes, UnsignedIntegerRoundTrip) {
    {
        Buffer buffer{};
        uint16_t integer{0xDEAD};
        serialize(buffer.data(), integer);
        ASSERT_EQ(integer, std::get<0>(deserialize<uint16_t>(buffer.data())));
    }
    {
        Buffer buffer{};
        uint32_t integer{0xDEADBEEF};
        serialize(buffer.data(), integer);
        ASSERT_EQ(integer, std::get<0>(deserialize<uint32_t>(buffer.data())));
    }
    {
        Buffer buffer{};
        uint64_t integer{0xDEADBEEFBABADEDA};
        serialize(buffer.data(), integer);
        ASSERT_EQ(integer, std::get<0>(deserialize<uint64_t>(buffer.data())));
    }
}

TEST(Serdes, ByteRoundTrip) {
    {
        Buffer buffer{};
        uint8_t byte{0xAB};
        serialize(buffer.data(), byte);
        ASSERT_EQ(byte, std::get<0>(deserialize<uint8_t>(buffer.data())));
    }
    {
        Buffer buffer{};
        std::byte byte{0xCD};
        serialize(buffer.data(), byte);
        ASSERT_EQ(byte, std::get<0>(deserialize<std::byte>(buffer.data())));
    }
    {
        Buffer buffer{};
        char byte{static_cast<char>(0xEF)};
        serialize(buffer.data(), byte);
        ASSERT_EQ(byte, std::get<0>(deserialize<char>(buffer.data())));
    }
}

TEST(Serdes, ArrayRoundTrip) {
    {
        Buffer buffer{};
        std::array<uint16_t, 3> array{1, 2, 3};
        serialize(buffer.data(), array);
        auto read_array = std::get<0>(deserialize<std::span<const uint16_t>>(buffer.data()));
        ASSERT_TRUE(std::ranges::equal(array, read_array));
    }
    {
        Buffer buffer{};
        std::array<uint16_t, 3> array{1, 2, 3};
        serialize(buffer.data(), array);
        auto read_array = std::get<0>(deserialize<std::vector<uint16_t>>(buffer.data()));
        ASSERT_TRUE(std::ranges::equal(array, read_array));
    }
    {
        Buffer buffer{};
        std::vector<uint16_t> array{1, 2, 3};
        serialize(buffer.data(), array);
        auto read_array = std::get<0>(deserialize<std::span<const uint16_t>>(buffer.data()));
        ASSERT_TRUE(std::ranges::equal(array, read_array));
    }
    {
        Buffer buffer{};
        std::vector<uint16_t> array{1, 2, 3};
        serialize(buffer.data(), array);
        auto read_array = std::get<0>(deserialize<std::vector<uint16_t>>(buffer.data()));
        ASSERT_TRUE(std::ranges::equal(array, read_array));
    }
    {
        Buffer buffer{};
        uint16_t array[]{1, 2, 3};
        serialize(buffer.data(), array);
        auto read_array = std::get<0>(deserialize<std::span<const uint16_t>>(buffer.data()));
        ASSERT_TRUE(std::ranges::equal(array, read_array));
    }
    {
        Buffer buffer{};
        uint16_t array[]{1, 2, 3};
        serialize(buffer.data(), array);
        auto read_array = std::get<0>(deserialize<std::vector<uint16_t>>(buffer.data()));
        ASSERT_TRUE(std::ranges::equal(array, read_array));
    }
}

} // namespace threesomeip_test