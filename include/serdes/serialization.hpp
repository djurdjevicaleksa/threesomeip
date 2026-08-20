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



#include <serdes/serdes_configuration.hpp>
#include <serdes/someip_types.hpp>
#include <serdes/someip_type_traits.hpp>


namespace threesomeip::someip::serdes {
using namespace threesomeip::someip;



/*
    Utilities
*/


template<typename T>
concept LeafConcept = Integer<T> || FloatingPoint<T> || Enum<T> || UTF8String<T>;

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
concept PaddedConcept = UTF8String<T> || Array<T>;


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
 *     DEFINITIONS      *
\*======================*/





/*
    String
*/







/*
    Array
*/


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

} // namespace threesomeip::someip::serdes

#endif // _SERIALIZATION_HPP