#ifndef _AGGREGATES_HPP
#define _AGGREGATES_HPP

/*=====*\
 * C++ *
\*=====*/
#include <cstddef>
#include <type_traits>
#include <cstdint>
#include <cstring>

/*=============*\
 * APPLICATION *
\*=============*/
#include <serdes/concepts.hpp>
#include <serdes/someip_type_traits.hpp>
#include <serdes/serdes_declarations.hpp>
#include <serdes/serdes_configuration.hpp>
#include <serdes/integers.hpp>


namespace threesomeip::someip::serdes {
using namespace threesomeip::someip;


template<Aggregate T>
consteval size_t _aggregate_field_count() {
    using A = traits::any_except<T>;

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
    else static_assert(traits::always_false<T>::value && "Either too many aggregate fields or none.");
}


template<typename T, typename Visitor>
void _visit_fields(T& object, const Visitor& v) {
    constexpr size_t field_count{_aggregate_field_count<T>()};

    if constexpr (field_count == 0) {
        static_assert(traits::always_false<T>::value && "Aggregate contains no fields.");
    }
    else if constexpr (field_count == 1) {
        auto& [a] = object;
        v(a);
    }
    else if constexpr (field_count == 2) {
        auto& [a, b] = object;
        v(a); v(b);
    }
    else if constexpr (field_count == 3) {
        auto& [a, b, c] = object;
        v(a); v(b); v(c);
    }
    else if constexpr (field_count == 4) {
        auto& [a, b, c, d] = object;
        v(a); v(b); v(c); v(d);
    }
    else if constexpr (field_count == 5) {
        auto& [a, b, c, d, e] = object;
        v(a); v(b); v(c); v(d); v(e);
    }
    else if constexpr (field_count == 6) {
        auto& [a, b, c, d, e, f] = object;
        v(a); v(b); v(c); v(d); v(e); v(f);
    }
    else if constexpr (field_count == 7) {
        auto& [a, b, c, d, e, f, g] = object;
        v(a); v(b); v(c); v(d); v(e); v(f); v(g);
    }
    else if constexpr (field_count == 8) {
        auto& [a, b, c, d, e, f, g, h] = object;
        v(a); v(b); v(c); v(d); v(e); v(f); v(g); v(h);
    }
    else if constexpr (field_count == 9) {
        auto& [a, b, c, d, e, f, g, h, i] = object;
        v(a); v(b); v(c); v(d); v(e); v(f); v(g); v(h); v(i);
    }
    else if constexpr (field_count == 10) {
        auto& [a, b, c, d, e, f, g, h, i, j] = object;
        v(a); v(b); v(c); v(d); v(e); v(f); v(g); v(h); v(i); v(j);
    }
    else {
        static_assert(traits::always_false<T>::value && "Aggregate contains too many fields.");
    }
}

const auto _serialization_visitor = [] (auto self, auto& out, auto& field) {
    if constexpr (Aggregate<decltype(field)>) {
        _visit_fields(field, [&] (auto& f) {
            self(self, out, f);
        });
    }
    else {
        _serialize(out, field);
    }
};

const auto _deserialization_visitor = [] (auto self, auto& in, auto& field) {
    if constexpr (Aggregate<decltype(field)>) {
        _visit_fields(field, [&] (auto& f) {
            self(self, in, f);
        });
    }
    else {
        field = _deserialize<std::remove_cvref_t<decltype(field)>>(in);
    }
};

const auto _serialization_dry_run_visitor = [] (auto self, auto& field) -> size_t {
    if constexpr (Aggregate<std::decay_t<decltype(field)>>) {
        size_t _size{0};
        _visit_fields(field, [&] (auto& f) {
            _size += self(self, f);
        });
        return _size;
    }
    else {
        return _serialize_dry_run(field);
    }
};

template<Aggregate T>
void _serialize(std::byte*& out, const T& agg) {
    if constexpr (! std::is_same_v<traits::aggregate_length_field_t<T>, void>) {
        traits::aggregate_length_field_t<T> length_bytes{convert_endianness<config::WIRE_BYTE_ORDER>(
            static_cast<traits::aggregate_length_field_t<T>>(_serialize_dry_run(agg) - sizeof(traits::aggregate_length_field_t<T>))
        )};
        std::memcpy(out, &length_bytes, sizeof(traits::aggregate_length_field_t<T>));
        out += sizeof(traits::aggregate_length_field_t<T>);
    }

    _visit_fields(agg, [&] (auto& field) {
        _serialization_visitor(_serialization_visitor, out, field);
    });
}

template<Aggregate T>
T _deserialize(std::byte*& in) {
    if constexpr (! std::is_same_v<traits::aggregate_length_field_t<T>, void>) {
        traits::aggregate_length_field_t<T> wire_length{0};
        std::memcpy(&wire_length, in, sizeof(traits::aggregate_length_field_t<T>));
        in += sizeof(traits::aggregate_length_field_t<T>);
        size_t length_bytes{std::size_t(convert_endianness<config::WIRE_BYTE_ORDER>(wire_length))};

        auto start = in;

        T agg{};
        _visit_fields(agg, [&] (auto& field) {
            _deserialization_visitor(_deserialization_visitor, in, field);
        });

        in = start + length_bytes;
        return agg;
    }
    else {
        T agg{};
        _visit_fields(agg, [&] (auto& field) {
            _deserialization_visitor(_deserialization_visitor, in, field);
        });
        return agg;
    }

}

template<Aggregate T>
size_t _serialize_dry_run(const T& aggregate) {
    size_t fields_size{0};
    _visit_fields(
        const_cast<T&>(aggregate),
        [&] (auto& field) {
            fields_size += _serialization_dry_run_visitor(_serialization_dry_run_visitor, field);
        }
    );

    if constexpr (std::is_same_v<traits::aggregate_length_field_t<T>, void>) {
        return fields_size;
    }
    else {
        return sizeof(traits::aggregate_length_field_t<T>) + fields_size;
    }
}

} // namespace threesomeip::someip::serdes

#endif // _AGGREGATES_HPP