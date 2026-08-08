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
#include <vector>
#include <span>


namespace threesomeip::ipc::serdes {

/*
    String type
*/
template<typename T>
concept String = std::convertible_to<std::remove_cvref_t<T>, std::string_view>
&& requires(T str) {
    { str.data() };
    { str.size() } -> std::convertible_to<size_t>;
};

template<String T>
void _serialize(std::byte*& out, T str) {
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
T _deserialize(std::byte*& in) {
    /* deserialize string length prefix */
    uint16_t string_length{0};
    std::memcpy(&string_length, in, sizeof(uint16_t));
    in += sizeof(uint16_t);

    /* deserialize string data */
    T str{reinterpret_cast<const char*>(in), string_length};
    in += string_length;
    return str;
}

/*
    Unsigned integer type
*/
template<typename T>
concept UnsignedInteger = std::is_same_v<std::remove_cvref_t<T>, uint16_t>
|| std::is_same_v<std::remove_cvref_t<T>, uint32_t>
|| std::is_same_v<std::remove_cvref_t<T>, uint64_t>;

template<UnsignedInteger T>
void _serialize(std::byte*& out, T num) {
    std::memcpy(out, &num, sizeof(T));
    out += sizeof(T);
}

template<UnsignedInteger T>
T _deserialize(std::byte*& in) {
    T num{0};
    std::memcpy(&num, in, sizeof(T));
    in += sizeof(T);
    return num;
}

/*
    Byte type
*/
template<typename T>
concept Byte = std::same_as<std::remove_cvref_t<T>, uint8_t>
    || std::same_as<std::remove_cvref_t<T>, char>
    || std::same_as<std::remove_cvref_t<T>, std::byte>;

template<Byte T>
void _serialize(std::byte*& out, T byte) {
    std::memcpy(out, &byte, sizeof(T));
    out += sizeof(T);
}

template<Byte T>
T _deserialize(std::byte*& in) {
    T byte{0};
    std::memcpy(&byte, in, sizeof(T));
    in += sizeof(T);
    return byte;
}

/*
    INTERNALS
*/
template<typename T>
concept _Serializable_Internal = String<T> || UnsignedInteger<T> || Byte<T>;



/*
    Array type
*/
template<typename T>
concept CStyleArray = (
    std::is_array_v<std::remove_cvref_t<T>>
    && !std::is_same_v<std::remove_extent_t<std::remove_cvref_t<T>>, char>
    && requires { requires _Serializable_Internal<std::remove_extent_t<std::remove_cvref_t<T>>>; }
    && !String<T>
);

template<typename T>
concept CppStyleArray = (
    std::ranges::contiguous_range<T>
    && requires(T t) {
        requires _Serializable_Internal<typename std::remove_cvref_t<T>::value_type>;
        t.data();
        t.size();
    }
    && !String<T>
);

template<typename T>
concept Array = CStyleArray<T> || CppStyleArray<T>;

template<Array T>
auto as_span(T&& arr) {
    if constexpr(CStyleArray<T>) return std::span{arr};
    else return std::span{arr.data(), arr.size()};
}

template<Array T>
void _serialize(std::byte*& out, T&& arr) {
    auto elements = as_span(std::forward<T>(arr)); /* makes a span */

    /* encode array length */
    uint16_t length = static_cast<uint16_t>(elements.size());
    std::memcpy(out, &length, sizeof(uint16_t));
    out += sizeof(uint16_t);

    /* encode array data */
    for (auto& element: elements) {
        _serialize(out, element);
    }
}

template<CppStyleArray T>
    requires std::same_as<T, std::vector<typename T::value_type>>
T _deserialize(std::byte*& in) {
    uint16_t array_length{0};
    std::memcpy(&array_length, in, sizeof(uint16_t));
    in += sizeof(uint16_t);

    T ret{};
    ret.reserve(array_length);
    for (uint16_t i{0}; i < array_length; ++i) {
        ret.push_back(_deserialize<typename T::value_type>(in));
    }
    return ret;
}

template<CppStyleArray T>
    requires std::same_as<T, std::span<const typename T::value_type>>
T _deserialize(std::byte*& in) {
    uint16_t array_length{0};
    std::memcpy(&array_length, in, sizeof(uint16_t));
    in += sizeof(uint16_t);

    auto* ptr = reinterpret_cast<const typename T::value_type*>(in);
    in += array_length * sizeof(typename T::value_type);
    return {ptr, array_length};
}


/*
Public API
*/
template<typename T>
concept Serializable = _Serializable_Internal<T> || Array<T>;

template<typename T>
concept Deserializable = _Serializable_Internal<T>
    || std::same_as<std::remove_cvref_t<T>, std::vector<typename T::value_type>>
    || std::same_as<std::remove_cvref_t<T>, std::span<const typename T::value_type>>;


template<Serializable... Args>
size_t serialize(std::byte*& out, Args&&... args) {
    const std::byte* const start = out;
    (_serialize(out, std::forward<Args>(args)), ...);
    return static_cast<size_t>(out - start);
}

template<Serializable... Args>
size_t serialize(std::byte* out, Args&&... args) {
    std::byte* cursor = out;
    (_serialize(cursor, std::forward<Args>(args)), ...);
    return static_cast<size_t>(cursor - out);
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
            (void(slots = _deserialize<std::remove_cvref_t<decltype(slots)>>(in)), ...);
        },
        ret
    );
    return ret;
}

template<Deserializable... Args>
std::tuple<Args...> deserialize(std::byte* in) {
    /*
        the standard does not guarantee std::tuple's constructor evaluates
        left to right, so the ordering has to be made explicit using std::apply
    */
    std::tuple<Args...> ret;
    std::apply(
        [&](auto&... slots) {
            (void(slots = _deserialize<std::remove_cvref_t<decltype(slots)>>(in)), ...);
        },
        ret
    );
    return ret;
}

} // namespace threesomeip::ipc::serdes

#endif // _SERIALIZATION_HPP