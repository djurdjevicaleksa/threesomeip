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
#include <cassert>


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
    || std::same_as<std::remove_cvref_t<T>, std::byte>
    || (std::is_enum_v<std::remove_cvref_t<T>> && std::convertible_to<std::underlying_type_t<std::remove_cvref_t<T>>, uint8_t>);

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
T _deserialize(std::byte*& in) {
    uint16_t array_length{0};
    std::memcpy(&array_length, in, sizeof(uint16_t));
    in += sizeof(uint16_t);

    if constexpr (std::same_as<T, std::span<const typename T::value_type>>) {
        auto* ptr = reinterpret_cast<const typename T::value_type*>(in);
        in += array_length * sizeof(typename T::value_type);
        return {ptr, array_length};
    }
    else if constexpr (std::same_as<T, std::vector<typename T::value_type>>) {
        T ret{};
        ret.reserve(array_length);
        for (uint16_t i{0}; i < array_length; ++i)
            ret.push_back(_deserialize<typename T::value_type>(in));
        return ret;
    }
    else {
        assert(array_length == std::tuple_size_v<T> && "Stored array is not the same size as the requested one");
        T ret{};
        for (auto& elem : ret)
            elem = _deserialize<typename T::value_type>(in);
        return ret;
    }
}

/*
    Aggregate types
*/
template<typename Exclude>
struct any_except {
    template<typename T>
        requires (!std::same_as<T, Exclude>)
    constexpr operator T() const noexcept;
};

template<typename T>
constexpr size_t _aggregate_member_variable_count() {
    using A = any_except<T>;
         if constexpr (requires { T{ A{}, A{}, A{}, A{}, A{}, A{}, A{}, A{}, A{}, A{}}; }) return size_t{10};
    else if constexpr (requires { T{ A{}, A{}, A{}, A{}, A{}, A{}, A{}, A{}, A{}}; }) return size_t{9};
    else if constexpr (requires { T{ A{}, A{}, A{}, A{}, A{}, A{}, A{}, A{}}; }) return size_t{8};
    else if constexpr (requires { T{ A{}, A{}, A{}, A{}, A{}, A{}, A{}}; }) return size_t{7};
    else if constexpr (requires { T{ A{}, A{}, A{}, A{}, A{}, A{}}; }) return size_t{6};
    else if constexpr (requires { T{ A{}, A{}, A{}, A{}, A{}}; }) return size_t{5};
    else if constexpr (requires { T{ A{}, A{}, A{}, A{}}; }) return size_t{4};
    else if constexpr (requires { T{ A{}, A{}, A{}}; }) return size_t{3};
    else if constexpr (requires { T{ A{}, A{}}; }) return size_t{2};
    else if constexpr (requires { T{ A{} }; }) return size_t{1};
    else return size_t{0};
}

template<typename T, typename Predicate>
void _visit_fields(T& object, Predicate&& p) {
    constexpr size_t field_count = _aggregate_member_variable_count<T>();
    if constexpr (field_count == 1) {
        const auto& [a] = object;
        p(a);
    }
    else if constexpr (field_count == 2) {
        const auto& [a, b] = object;
        p(a); p(b);
    }
    else if constexpr (field_count == 3) {
        const auto& [a, b, c] = object;
        p(a); p(b); p(c);
    }
    else if constexpr (field_count == 4) {
        const auto& [a, b, c, d] = object;
        p(a); p(b); p(c); p(d);
    }
    else if constexpr (field_count == 5) {
        const auto& [a, b, c, d, e] = object;
        p(a); p(b); p(c); p(d); p(e);
    }
    else if constexpr (field_count == 6) {
        const auto& [a, b, c, d, e, f] = object;
        p(a); p(b); p(c); p(d); p(e); p(f);
    }
    else if constexpr (field_count == 7) {
        const auto& [a, b, c, d, e, f, g] = object;
        p(a); p(b); p(c); p(d); p(e); p(f); p(g);
    }
    else if constexpr (field_count == 8) {
        const auto& [a, b, c, d, e, f, g, h] = object;
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h);
    }
    else if constexpr (field_count == 9) {
        const auto& [a, b, c, d, e, f, g, h, i] = object;
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h); p(i);
    }
    else if constexpr (field_count == 10) {
        const auto& [a, b, c, d, e, f, g, h, i, j] = object;
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h); p(i); p(j);
    }
}

template<typename T>
concept Aggregate = (
    std::is_aggregate_v<std::remove_cvref_t<T>>
    && std::is_class_v<std::remove_cvref_t<T>>
    && !std::is_array_v<std::remove_cvref_t<T>>
    && !Array<std::remove_cvref_t<T>>
);

template<Aggregate T>
void _serialize(std::byte*& out, const T& aggregate);

const auto _ser_visitor = [](auto self, auto& out, auto& field) {
    using field_t = std::decay_t<decltype(field)>;
    if constexpr (Aggregate<field_t>) {
        _visit_fields(field, [&](const auto& f) { self(self, out, f); });
    }
    else {
        _serialize(out, field);
    }
};

template<Aggregate T>
void _serialize(std::byte*& out, const T& aggregate) {
    _visit_fields(aggregate, [&] (const auto& field) {
        _ser_visitor(_ser_visitor, out, field);
    });
}

template<typename T, typename Predicate>
void _visit_fields_mutable(T& object, Predicate&& p) {
    constexpr size_t field_count = _aggregate_member_variable_count<T>();
    if constexpr (field_count == 1) {
        auto& [a] = object;
        p(a);
    }
    else if constexpr (field_count == 2) {
        auto& [a, b] = object;
        p(a); p(b);
    }
    else if constexpr (field_count == 3) {
        auto& [a, b, c] = object;
        p(a); p(b); p(c);
    }
    else if constexpr (field_count == 4) {
        auto& [a, b, c, d] = object;
        p(a); p(b); p(c); p(d);
    }
    else if constexpr (field_count == 5) {
        auto& [a, b, c, d, e] = object;
        p(a); p(b); p(c); p(d); p(e);
    }
    else if constexpr (field_count == 6) {
        auto& [a, b, c, d, e, f] = object;
        p(a); p(b); p(c); p(d); p(e); p(f);
    }
    else if constexpr (field_count == 7) {
        auto& [a, b, c, d, e, f, g] = object;
        p(a); p(b); p(c); p(d); p(e); p(f); p(g);
    }
    else if constexpr (field_count == 8) {
        auto& [a, b, c, d, e, f, g, h] = object;
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h);
    }
    else if constexpr (field_count == 9) {
        auto& [a, b, c, d, e, f, g, h, i] = object;
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h); p(i);
    }
    else if constexpr (field_count == 10) {
        auto& [a, b, c, d, e, f, g, h, i, j] = object;
        p(a); p(b); p(c); p(d); p(e); p(f); p(g); p(h); p(i); p(j);
    }
}

template<Aggregate T>
T _deserialize(std::byte*& in);

const auto _des_visitor = [](auto self, auto& in, auto& field) {
    using field_t = std::decay_t<decltype(field)>;
    if constexpr (Aggregate<field_t>) {
        _visit_fields_mutable(field, [&](auto& f) { self(self, in, f); });
    }
    else {
        field = _deserialize<field_t>(in);
    }
};

template<Aggregate T>
T _deserialize(std::byte*& in) {
    T aggregate{};
    _visit_fields_mutable(aggregate, [&](auto& field) {
        _des_visitor(_des_visitor, in, field);
    });
    return aggregate;
}



/*
Public API
*/
template<typename T>
concept Serializable = _Serializable_Internal<T> || Array<T> || Aggregate<T>;

template<typename T>
concept Deserializable = _Serializable_Internal<T>
    || std::same_as<std::remove_cvref_t<T>, std::vector<typename T::value_type>>
    || std::same_as<std::remove_cvref_t<T>, std::span<const typename T::value_type>>
    || Aggregate<T>;


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