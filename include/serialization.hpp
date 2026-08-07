#ifndef _SERIALIZATION_HPP
#define _SERIALIZATION_HPP

/*=====*\
 * C++ *
\*=====*/
#include <type_traits>
#include <cstddef>
#include <ranges>
#include <cstdint>
#include <cstring>


namespace threesomeip::ipc::serdes {


template<typename T>
concept String = std::convertible_to<std::remove_cvref_t<T>, std::string_view>
&& requires(T str) {
    { str.data() };
    { str.size() } -> std::convertible_to<size_t>;
};

template<typename T>
concept UnsignedInteger = std::unsigned_integral<std::remove_cvref_t<T>>
    && !std::is_same_v<std::remove_cvref_t<T>, bool>;

template<typename T>
concept Byte = std::same_as<std::remove_cvref_t<T>, uint8_t>
    || std::same_as<std::remove_cvref_t<T>, char>
    || std::same_as<std::remove_cvref_t<T>, std::byte>;

template<typename T>
concept Array = std::ranges::contiguous_range<T>
/* TODO */


template<typename T>
concept Serializable = String<T> || UnsignedInteger<T> || Byte<T>;

template<typename T>
concept Deserializable = Serializable<T>;


template<String T>
void serialize(std::byte*& out, T str) {
    /* serialize string length prefix */
    uint16_t string_length = static_cast<uint16_t>(str.size());
    std::memcpy(out, &string_length, sizeof(uint16_t));
    out += sizeof(uint16_t);

    /* serialize string data */
    std::memcpy(out, str.data(), str.size());
    out += str.size();
}

/* will potentially need to return std::string as owning object */
template<String T>
T deserialize(std::byte*& in) {
    /* deserialize string length prefix */
    uint16_t string_length{0};
    std::memcpy(&string_length, in, sizeof(uint16_t));
    in += sizeof(uint16_t);

    /* deserialize string data */
    T str{reinterpret_cast<const char*>(in), string_length};
    in += string_length;
    return str;
}


template<UnsignedInteger T>
void serialize(std::byte*& out, T num) {
    std::memcpy(out, &num, sizeof(T));
    out += sizeof(T);
}

template<UnsignedInteger T>
T deserialize(std::byte*& in) {
    T num{0};
    std::memcpy(&num, in, sizeof(T));
    in += sizeof(T);
    return num;
}


template<Byte T>
void serialize(std::byte*& out, T byte) {
    std::memcpy(out, &num, sizeof(T));
    out += sizeof(T);
}

template<Byte T>
T deserialize(std::byte*& in) {
    T byte{0};
    std::memcpy(&byte, in, sizeof(T));
    in += sizeof(T);
    return byte;
}


/*
    Public API
*/


template<Serializable... Args>
size_t serialize(std::byte*& out, Args&&... args) {
    const std::byte* const start = out;
    (serialize(out, std::forward<Args>(args)), ...);
    return size_t{out - start};
}

template<Serializable... Args>
size_t serialize(std::byte* out, Args&&... args) {
    std::byte* cursor = out;
    return serialize<Args...>(cursor, std::forward<Args>(args)...);
}

template<Deserializable... Args>
std::tuple<Args...> deserialize(std::byte*& in) {
    /*
        the standard does not guarantee std::tuple's constructor evaluates
        left to right, so the ordering has to be made explicit using std::apply
    */
    std::tuple<Args...> ret;
    std::apply(
        [&](auto&... slots) {
            (void(slots = deserialize<decltype(slots)>(in)), ...);
        },
        ret
    );
    return ret;
}

template<Deserializable... Args>
std::tuple<Args...> deserialize(std::byte* in) {
    std::byte* cursor = in;
    return deserialize<Args...>(cursor);
}


/* TESTS */
static_assert(String<std::string>);
static_assert(String<std::string_view>);
static_assert(!String<char[]>);
static_assert(!String<char*>);

static_assert(UnsignedInteger<uint8_t>);
static_assert(UnsignedInteger<uint32_t&>);
static_assert(UnsignedInteger<const uint64_t>);
static_assert(!UnsignedInteger<uint32_t[]>);
static_assert(!UnsignedInteger<int32_t[]>);
static_assert(!UnsignedInteger<int8_t>);
static_assert(!UnsignedInteger<const int32_t>);

static_assert(Byte<uint8_t>);
static_assert(Byte<const std::byte>);
static_assert(Byte<char&>);
static_assert(!Byte<int8_t>);
static_assert(!Byte<uint16_t>);
} // namespace threesomeip::ipc::serdes

#endif // _SERIALIZATION_HPP