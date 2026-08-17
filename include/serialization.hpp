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
    Concepts
*/

template<typename T>
concept UnsignedInteger =
       std::is_same_v<std::remove_cvref_t<T>, uint16_t>
    || std::is_same_v<std::remove_cvref_t<T>, uint32_t>
    || std::is_same_v<std::remove_cvref_t<T>, uint64_t>;

template<typename T>
concept SignedInteger =
       std::is_same_v<std::remove_cvref_t<T>, int16_t>
    || std::is_same_v<std::remove_cvref_t<T>, int32_t>
    || std::is_same_v<std::remove_cvref_t<T>, int64_t>;

template<typename T>
concept Integer = UnsignedInteger<T> || SignedInteger<T>;


template<typename T>
concept Byte =
       std::same_as<std::remove_cvref_t<T>, uint8_t>
    || std::same_as<std::remove_cvref_t<T>, int8_t>
    || std::same_as<std::remove_cvref_t<T>, std::byte>;


template<typename T>
concept Enum =
       std::is_enum_v<std::remove_cvref_t<T>>
    && !Byte<T>
    && (
           Integer<std::underlying_type_t<std::remove_cvref_t<T>>>
        || Byte<std::underlying_type_t<std::remove_cvref_t<T>>>
    );


template<typename T>
concept String =
       std::convertible_to<std::remove_cvref_t<T>, std::string_view>
    && requires(T str) {
        { str.data() };
        { str.size() } -> std::convertible_to<size_t>;
    };

template<typename T>
concept CStyleArray =
       std::is_array_v<std::remove_cvref_t<T>>
    && !std::is_same_v<std::remove_extent_t<std::remove_cvref_t<T>>, char> // explicitly ban char[]
    && !String<T>;

template<typename T>
concept CppStyleArray = (
    std::ranges::contiguous_range<T>
    && requires(T t) {
        t.data();
        t.size();
    }
    && !String<T>
);

template<typename T>
concept Array =
       CStyleArray<T>
    || CppStyleArray<T>;

template<typename T>
concept Aggregate =
       std::is_aggregate_v<std::remove_cvref_t<T>>
    && std::is_class_v<std::remove_cvref_t<T>>
    && !Array<T>; /* must explicitly differentiate */

/*
    Utilities
*/

template<typename T>
struct always_false : std::false_type {};

template<Array T>
using array_value_t = std::conditional_t<
    CStyleArray<T>,
    std::remove_extent_t<std::remove_cvref_t<T>>,
    std::ranges::range_value_t<T>
>;

template<typename T>
concept LeafConcept = Integer<T> || Byte<T> || Enum<T> || String<T>;

template<typename T>
constexpr bool is_serializable_v();

template<typename Excluded>
struct any_except {
    template<typename T>
        requires (!std::same_as<T, Excluded>)
    constexpr operator T() const noexcept;
};

/* aggregates of up to 10 elements are supported */
template<typename T>
constexpr size_t _aggregate_field_count() {
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
    else static_assert(always_false<T>::value && "Either too much aggregate fields or none.");
}


template<typename T>
constexpr bool _aggregate_fields_serializable() {
    constexpr size_t field_count = _aggregate_field_count<T>();

    if constexpr (field_count == 0) {
        return true;
    }
    else {
        T object{};

        if constexpr (field_count == 1) {
            auto& [a] = object;
            return is_serializable_v<std::remove_cvref_t<decltype(a)>>();
        }
        else if constexpr (field_count == 2) {
            auto& [a, b] = object;
            return is_serializable_v<std::remove_cvref_t<decltype(a)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(b)>>();
        }
        else if constexpr (field_count == 3) {
            auto& [a, b, c] = object;
            return is_serializable_v<std::remove_cvref_t<decltype(a)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(b)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(c)>>();
        }
        else if constexpr (field_count == 4) {
            auto& [a, b, c, d] = object;
            return is_serializable_v<std::remove_cvref_t<decltype(a)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(b)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(c)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(d)>>();
        }
        else if constexpr (field_count == 5) {
            auto& [a, b, c, d, e] = object;
            return is_serializable_v<std::remove_cvref_t<decltype(a)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(b)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(c)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(d)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(e)>>();
        }
        else if constexpr (field_count == 6) {
            auto& [a, b, c, d, e, f] = object;
            return is_serializable_v<std::remove_cvref_t<decltype(a)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(b)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(c)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(d)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(e)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(f)>>();
        }
        else if constexpr (field_count == 7) {
            auto& [a, b, c, d, e, f, g] = object;
            return is_serializable_v<std::remove_cvref_t<decltype(a)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(b)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(c)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(d)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(e)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(f)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(g)>>();
        }
        else if constexpr (field_count == 8) {
            auto& [a, b, c, d, e, f, g, h] = object;
            return is_serializable_v<std::remove_cvref_t<decltype(a)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(b)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(c)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(d)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(e)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(f)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(g)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(h)>>();
        }
        else if constexpr (field_count == 9) {
            auto& [a, b, c, d, e, f, g, h, i] = object;
            return is_serializable_v<std::remove_cvref_t<decltype(a)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(b)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(c)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(d)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(e)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(f)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(g)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(h)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(i)>>();
        }
        else if constexpr (field_count == 10) {
            auto& [a, b, c, d, e, f, g, h, i, j] = object;
            return is_serializable_v<std::remove_cvref_t<decltype(a)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(b)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(c)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(d)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(e)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(f)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(g)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(h)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(i)>>()
                && is_serializable_v<std::remove_cvref_t<decltype(j)>>();
        }
        else return false;
    }
}

template<typename T>
constexpr bool is_serializable_v() {
    using U = std::remove_cvref_t<T>;

    if constexpr (LeafConcept<U>) {
        return true;
    }
    else if constexpr (Array<U>) {
        return is_serializable_v<array_value_t<U>>();
    }
    else if constexpr (Aggregate<U>) {
        return _aggregate_fields_serializable<U>();
    }
    else return false;
}

template<typename T>
concept StructuralConcept = Array<T> || Aggregate<T>;


template<typename T>
concept Serializable =
       LeafConcept<T>
    || (Array<T> && is_serializable_v<array_value_t<T>>())
    || (Aggregate<T> && is_serializable_v<T>());

template<typename T>
concept Deserializable =
       LeafConcept<T>
    || (CppStyleArray<T> && is_serializable_v<array_value_t<T>>())
    || (Aggregate<T> && is_serializable_v<T>());


/*======================*\
 * FORWARD DECLARATIONS *
\*======================*/


template<Integer T>
void _serialize(std::byte*& out, const T& num);

template<Integer T>
T _deserialize(std::byte*& in);

template<Byte T>
void _serialize(std::byte*& out, const T& enu);

template<Byte T>
T _deserialize(std::byte*& in);

template<Enum T>
void _serialize(std::byte*& out, const T& byt);

template<Enum T>
T _deserialize(std::byte*& in);

template<String T>
void _serialize(std::byte*& out, const T& str);

template<String T>
T _deserialize(std::byte*& in);

template<Array T>
void _serialize(std::byte*& out, const T& arr);

template<CppStyleArray T>
T _deserialize(std::byte*& in);

template<Aggregate T>
void _serialize(std::byte*& out, const T& agg);

template<Aggregate T>
T _deserialize(std::byte*& in);


/*======================*\
 *     DEFINITIONS      *
\*======================*/

/*
    Integer
*/
template<Integer T>
void _serialize(std::byte*& out, const T& num) {
    std::memcpy(out, &num, sizeof(T));
    out += sizeof(T);
}

template<Integer T>
T _deserialize(std::byte*& in) {
    T num{0};
    std::memcpy(&num, in, sizeof(T));
    in += sizeof(T);
    return num;
}

template<Integer T>
constexpr size_t _serialize_dry_run() {
    return sizeof(T);
}

template<Integer T>
constexpr size_t _serialize_dry_run([[maybe_unused]] const T& num) {
    return sizeof(T);
}

/*
    Byte
*/
template<Byte T>
void _serialize(std::byte*& out, const T& byt) {
    std::memcpy(out, &byt, sizeof(T));
    out += sizeof(T);
}

template<Byte T>
T _deserialize(std::byte*& in) {
    T byte{0};
    std::memcpy(&byte, in, sizeof(T));
    in += sizeof(T);
    return byte;
}

template<Byte T>
constexpr size_t _serialize_dry_run() {
    return sizeof(T);
}

template<Byte T>
constexpr size_t _serialize_dry_run([[maybe_unused]] const T& byt) {
    return sizeof(T);
}

/*
    Enum
*/
template<Enum T>
void _serialize(std::byte*& out, const T& enu) {
    std::memcpy(out, &enu, sizeof(std::underlying_type_t<std::remove_cvref_t<T>>));
    out += sizeof(std::underlying_type_t<std::remove_cvref_t<T>>);
}

template<Enum T>
T _deserialize(std::byte*& in) {
    T enu{0};
    std::memcpy(&enu, in, sizeof(std::underlying_type_t<std::remove_cvref_t<T>>));
    in += sizeof(std::underlying_type_t<std::remove_cvref_t<T>>);
    return enu;
}

template<Enum T>
constexpr size_t _serialize_dry_run() {
    return sizeof(std::underlying_type_t<std::remove_cvref_t<T>>);
}

template<Enum T>
constexpr size_t _serialize_dry_run([[maybe_unused]] const T& enu) {
    return sizeof(std::underlying_type_t<std::remove_cvref_t<T>>);
}

/*
    String
*/
template<String T>
void _serialize(std::byte*& out, const T& str) {
    uint16_t length = static_cast<uint16_t>(str.size());
    std::memcpy(out, &length, sizeof(uint16_t));
    out += sizeof(uint16_t);

    std::memcpy(out, str.data(), str.size());
    out += str.size();
}

/* will potentially need to return std::string as owning object */
template<String T>
T _deserialize(std::byte*& in) {
    uint16_t length{0};
    std::memcpy(&length, in, sizeof(uint16_t));
    in += sizeof(uint16_t);

    T str{reinterpret_cast<const char*>(in), length};
    in += length;
    return str;
}

template<String T>
size_t _serialize_dry_run(const T& str) {
    return sizeof(uint16_t) + str.size();
}

/*
    Array
*/
template<Array T>
auto as_span(const T& arr) {
    if constexpr(CStyleArray<T>) {
        return std::span{arr};
    }
    else {
        return std::span{arr.data(), arr.size()};
    }
}

template<Array T>
void _serialize(std::byte*& out, const T& arr) {
    auto elements = as_span(arr);

    uint16_t length = static_cast<uint16_t>(elements.size());
    std::memcpy(out, &length, sizeof(uint16_t));
    out += sizeof(uint16_t);

    for (const auto element: elements) {
        _serialize(out, element);
    }
}

template<CppStyleArray T>
T _deserialize(std::byte*& in) {
    uint16_t length{0};
    std::memcpy(&length, in, sizeof(uint16_t));
    in += sizeof(uint16_t);

    if constexpr (std::same_as<std::remove_cvref_t<T>, std::span<const typename std::remove_cvref_t<T>::value_type>>) {
        auto ptr = reinterpret_cast<const typename T::value_type*>(in);
        in += length * sizeof(typename std::remove_cvref_t<T>::value_type);
        return {ptr, length};
    }
    else if constexpr (std::same_as<std::remove_cvref_t<T>, std::vector<typename std::remove_cvref_t<T>::value_type>>) {
        T arr{};
        arr.reserve(length);
        for (uint16_t i{0}; i < length; ++i) {
            arr.emplace_back(_deserialize<typename std::remove_cvref_t<T>::value_type>(in));
        }
        return arr;
    }
    else { // std::array
        assert(length == std::tuple_size_v<std::remove_cvref_t<T>> && "Mismatch between a written and read array detected. What did you do?");
        T arr{};
        for (auto& element: arr) {
            element = _deserialize<typename std::remove_cvref_t<T>::value_type>(in);
        }
        return arr;
    }
}

template<Array T>
size_t _serialize_dry_run(const T& arr) {
    const auto arr_as_span = as_span(arr);
    return sizeof(uint16_t) + arr_as_span.size() * sizeof(typename std::remove_cvref_t<decltype(arr_as_span)>::value_type);
}

/*
    Aggregate
*/
template<typename T, typename Predicate>
void _visit_fields(T& object, const Predicate& p) {
    constexpr size_t field_count = _aggregate_field_count<T>();

    if constexpr (field_count == 0) {
        static_assert(always_false<T>::value && "Aggregate contains no fields.");
    }
    else if constexpr (field_count == 1) {
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
    else {
        static_assert(always_false<T>::value && "Aggregate contains too many fields.");
    }
}

const auto _serialization_visitor = [] (auto self, auto& out, auto& field) {
    if constexpr (Aggregate<decltype(field)>) {
        _visit_fields(
            field,
            [&] (auto& f) {
                self(self, out, f);
            }
        );
    }
    else {
        _serialize(out, field);
    }
};

const auto _deserialization_visitor = [] (auto self, auto& in, auto& field) {
    if constexpr (Aggregate<decltype(field)>) {
        _visit_fields(
            field,
            [&] (auto& f) {
                self(self, in, f);
            }
        );
    }
    else {
        field = _deserialize<std::remove_cvref_t<decltype(field)>>(in);
    }
};

const auto _serialization_dry_run_visitor = [] (auto self, auto& field) -> size_t {
    if constexpr (Aggregate<std::decay_t<decltype(field)>>) {
        size_t _size{0};
        _visit_fields(
            field,
            [&] (auto& f) {
                _size += self(self, f);
            }
        );
        return _size;
    }
    else {
        return _serialize_dry_run(field);
    }
};

template<Aggregate T>
void _serialize(std::byte*& out, const T& agg) {
    _visit_fields(
        agg,
        [&] (auto& field) {
            _serialization_visitor(_serialization_visitor, out, field);
        }
    );
}

template<Aggregate T>
T _deserialize(std::byte*& in) {
    T agg{};
    _visit_fields(
        agg,
        [&] (auto& field) {
            _deserialization_visitor(_deserialization_visitor, in, field);
        }
    );
    return agg;
}

template<Aggregate T>
size_t _serialize_dry_run(const T& aggregate) {
    size_t _size{0};
    _visit_fields(
        const_cast<T&>(aggregate),
        [&] (auto& field) {
            _size += _serialization_dry_run_visitor(_serialization_dry_run_visitor, field);
        }
    );
    return _size;
}


/*======================*\
 *     PUBLIC API       *
\*======================*/


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
auto deserialize(const std::byte* in, std::byte** out_cursor = nullptr) {
    /*
        Force correct sequential evaluation of deserialization statements
        because they share and modify state (in)
    */
    std::byte* cursor = const_cast<std::byte*>(in);
    std::tuple<Args...> ret;
    std::apply(
        [&](auto&... slots) {
            (void(slots = _deserialize<std::remove_cvref_t<decltype(slots)>>(cursor)), ...);
        },
        ret
    );

    if (out_cursor) *out_cursor = cursor;

    if constexpr (sizeof...(Args) == 1) {
        return std::get<0>(ret);
    }
    else return ret;
}


template<Serializable... Args>
size_t serialize_dry_run(Args&&... args) {
    return (_serialize_dry_run(std::forward<Args>(args)) + ... + size_t{0});
}

} // namespace threesomeip::ipc::serdes

#endif // _SERIALIZATION_HPP