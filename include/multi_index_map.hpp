#ifndef _MULTI_INDEX_MAP
#define _MULTI_INDEX_MAP


#include <unordered_map>
#include <type_traits>
#include <cstddef>
#include <tuple>
#include <utility>
#include <concepts>
#include <vector>


namespace threesomeip::utils {


template<typename T>
concept StdHashable = requires (T a) {
    { std::hash<std::remove_cvref_t<T>>{}(a) } -> std::convertible_to<size_t>;
};

template<typename T>
concept CustomHashable = requires (T a) {
    { typename std::remove_cvref_t<T>::hash{}(a) } -> std::convertible_to<size_t>;
};

template<typename T>
concept Hashable = StdHashable<T> || CustomHashable<T>;

template<typename T>
struct hash_functor_impl {};

template<StdHashable T>
struct hash_functor_impl<T> {
    using type = std::hash<typename std::remove_cvref_t<T>>;
};

template<CustomHashable T>
struct hash_functor_impl<T> {
    using type = typename std::remove_cvref_t<T>::hash;
};

template<typename T>
using hash_functor_t = typename hash_functor_impl<T>::type;




template<typename Value, typename PrimaryKey, typename... SecondaryKeys>
requires (
    Hashable<PrimaryKey> && (Hashable<SecondaryKeys> && ...)
)
class multi_index_unordered_map_t {
public:
    struct Record;

    template<typename Key>
    using secondary_map_t = std::unordered_map<Key, Record*, hash_functor_t<Key>>;
    using secondary_maps_t = std::tuple<secondary_map_t<SecondaryKeys>...>;

    struct Record {
        PrimaryKey primary_key;
        Value value;
        std::tuple<std::vector<SecondaryKeys>...> secondaries;
    };

    using primary_map_t = std::unordered_map<PrimaryKey, Record, hash_functor_t<PrimaryKey>>;


    multi_index_unordered_map_t() = default;


    Value* insert(PrimaryKey pkey, Value value) {
        if (m_primary.contains(pkey)) return nullptr;

        Record record{pkey, std::move(value), {} };
        auto [it, _] = m_primary.emplace(std::move(pkey), std::move(record));
        return &(it->second.value);
    }

    Value* insert(PrimaryKey pkey, Value value, SecondaryKeys... skeys) {
        Value* v_ptr = this->insert(pkey, std::move(value));
        if (nullptr == v_ptr) return nullptr;

        if ( !this->addAliases(pkey, std::move(skeys)...)) {
            this->erase(pkey);
            return nullptr;
        }
        return v_ptr;
    }

    template<typename Key>
    bool alias(const PrimaryKey& primary_key, const Key& secondary_key) {
        using KeyBaseType = std::decay_t<Key>;
        static_assert((std::is_same_v<KeyBaseType, SecondaryKeys> || ...), "Provided key is not one of the secondary keys.");

        auto it = m_primary.find(primary_key);
        if (it == m_primary.end()) return false;

        auto& map = std::get<secondary_map_t<KeyBaseType>>(m_secondaries);
        if (map.contains(secondary_key)) return false;

        Record* record = &(it->second);
        map.emplace(std::move(secondary_key), record);
        std::get<std::vector<KeyBaseType>>(record->secondaries).push_back(secondary_key);
        return true;
    }

    template<typename Key>
    bool remove_alias(const Key& key) {
        using KeyBaseType = std::decay_t<Key>;
        static_assert((std::is_same_v<KeyBaseType, SecondaryKeys> || ...), "Provided key is not one of the secondary keys.");

        auto& map = std::get<secondary_map_t<KeyBaseType>>(m_secondaries);
        auto it = map.find(key);
        if (it == map.end()) return false;

        auto& keys = std::get<std::vector<KeyBaseType>>(it->second->secondaries);
        std::erase(keys, key);
        map.erase(it);
        return true;
    }


    template<typename Key>
    auto* find(this auto&& self, const Key& key) {
        using KeyBaseType = std::decay_t<Key>;

        if constexpr (std::is_same_v<KeyBaseType, PrimaryKey>) {
            auto it = self.m_primary.find(key);
            return it == self.m_primary.end() ? nullptr : &(it->second->value);
        }
        else if constexpr ((std::is_same_v<KeyBaseType, SecondaryKeys> || ...)) {
            auto& map = std::get<secondary_map_t<KeyBaseType>>(self.m_secondaries);
            auto it = map.find(key);
            return it == map.end() ? nullptr : &(it->second->value);
        }
        else {
            static_assert(!sizeof(Key*), "Provided key is not the primary nor any of the secondary keys.");
        }
    }

    template<typename Key>
    bool erase(const Key& key) {
        using KeyBaseType = std::decay_t<Key>;

        if constexpr (std::is_same_v<KeyBaseType, PrimaryKey>) {
            auto it = m_primary.find(key);
            if (it == m_primary.end()) return false;
            this->eraseAllSecondaries(it->second, std::index_sequence_for<SecondaryKeys...>{});
            m_primary.erase(it);
            return true;
        }
        else if constexpr ((std::is_same_v<KeyBaseType, SecondaryKeys> || ...)) {
            auto& map = std::get<secondary_map_t<KeyBaseType>>(m_secondaries);
            auto it = map.find(key);
            return it == map.end() ? false : this->erase(it->second->primary_key);
        }
        else {
            static_assert(!sizeof(Key*), "Provided key is not the primary nor any of the secondary keys.");
        }
    }

    template<typename Key>
    bool contains(const Key& key) {
        using KeyBaseType = std::decay_t<Key>;

        if constexpr (std::is_same_v<KeyBaseType, PrimaryKey>) {
            return m_primary.contains(key);
        }
        else if constexpr ((std::is_same_v<KeyBaseType, SecondaryKeys> || ...)) {
            return std::get<secondary_map_t<KeyBaseType>>(m_secondaries).contains(key);
        }
        else {
            static_assert(!sizeof(Key*), "Provided key is not the primary nor any of the secondary keys.");
        }
    }

    template<typename Key>
    Value& at(const Key& key) {
        using KeyBaseType = std::decay_t<Key>;

        if constexpr (std::is_same_v<KeyBaseType, PrimaryKey>) {
            return m_primary.at(key).value;
        }
        else if constexpr ((std::is_same_v<KeyBaseType, SecondaryKeys> || ...)) {
            return std::get<secondary_map_t<KeyBaseType>>(m_secondaries).at(key)->value;
        }
        else {
            static_assert(!sizeof(Key*), "Provided key is not the primary nor any of the secondary keys.");
        }
    }

private:

    bool addAliases(const PrimaryKey&, SecondaryKeys...)
    requires (sizeof...(SecondaryKeys) == 0) {
        return true;
    }

    bool addAliases(const PrimaryKey& pkey, SecondaryKeys... skeys) {
        return ( this->template alias<SecondaryKeys>(pkey, std::move(skeys)) && ...);
    }

    template<size_t... Is>
    void eraseAllSecondaries(Record& record, std::index_sequence<Is...>) {
        ( eraseSecondary<Is>(record), ...);
    }

    template<size_t I>
    void eraseSecondary(Record& record) {
        auto& keys = std::get<I>(record.secondaries);
        auto& map = std::get<I>(m_secondaries);
        for (auto& key: keys) {
            map.erase(key);
        }
        keys.clear();
    }


    primary_map_t m_primary;
    secondary_maps_t m_secondaries;
};

} // namespace threesomeip::utils

#endif // _MULTI_INDEX_MAP