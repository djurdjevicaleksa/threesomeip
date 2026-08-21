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
#include <bit>
#include <array>

/*========*\
 * SERDES *
\*========*/
#include <serdes/concepts.hpp>
#include <serdes/integers.hpp>
#include <serdes/floats.hpp>
#include <serdes/enums.hpp>
#include <serdes/strings.hpp>
#include <serdes/arrays.hpp>
#include <serdes/aggregates.hpp>
#include <serdes/someip_types.hpp>
#include <serdes/serdes_configuration.hpp>
#include <serdes/someip_type_traits.hpp>


namespace threesomeip::someip::serdes {
using namespace threesomeip::someip;

/*
    Utilities
*/

template<typename T>
constexpr bool is_serializable_v();

template<typename T>
constexpr bool _aggregate_fields_serializable() {
    constexpr size_t field_count{_aggregate_field_count<T>()};

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
        return is_serializable_v<traits::array_value_t<U>>();
    }
    else if constexpr (Aggregate<U>) {
        return _aggregate_fields_serializable<U>();
    }
    else return false;
}

template<typename T>
concept Serializable =
       LeafConcept<T>
    || (Array<T> && is_serializable_v<traits::array_value_t<T>>())
    || (Aggregate<T> && is_serializable_v<T>());

template<typename T>
concept Deserializable = Serializable<T>;


/*======================*\
 *     PUBLIC API       *
\*======================*/


template<Serializable... Args>
size_t serialize(std::byte* out, Args&&... args) {
    const auto start = out;
    (_serialize(out, std::forward<Args>(args)), ...);
    return static_cast<size_t>(out - start);
}

// template<Serializable... Args>
// size_t serialize(std::byte* out, Args&&... args) {
//     std::byte* cursor = out;
//     (_serialize(cursor, std::forward<Args>(args)), ...);
//     return static_cast<size_t>(cursor - out);
// }

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
requires (requires { _serialize_dry_run<Args>(); } && ...)
consteval size_t serialize_dry_run() {
    return (_serialize_dry_run<Args>() + ... + size_t{0});
}

template<Serializable... Args>
size_t serialize_dry_run(Args&&... args) {
    return (_serialize_dry_run(std::forward<Args>(args)) + ... + size_t{0});
}

} // namespace threesomeip::someip::serdes

#endif // _SERIALIZATION_HPP