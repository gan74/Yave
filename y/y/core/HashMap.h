/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef Y_CORE_HASHMAP_H
#define Y_CORE_HASHMAP_H

#include "Range.h"
#include "FixedArray.h"

#include <y/utils/hash.h>
#include <y/utils/traits.h>

#include <functional>


Y_TODO("Implement trick from here: https://www.youtube.com/watch?v=ncHmEUmJZf4")

namespace y {
namespace core {

// http://research.cs.vt.edu/AVresearch/hashing/index.php

Y_TODO(Maybe keep track of the first element index and start iterating from here)
Y_TODO(We check the load factor before adding elements so we can end up slightly above after insertion)

namespace detail {
template<typename K, typename V>
inline const std::pair<const K, V>* map_entry_to_value_type_ptr(const std::pair<K, V>* p) {
    return reinterpret_cast<const std::pair<const K, V>*>(p);
}

template<typename K, typename V>
inline std::pair<const K, V>* map_entry_to_value_type_ptr(std::pair<K, V>* p) {
    return reinterpret_cast<std::pair<const K, V>*>(p);
}

template<typename K, typename V>
inline const std::pair<const K, V>& map_entry_to_value_type(const std::pair<K, V>& p) {
    return *map_entry_to_value_type_ptr(&p);
}

template<typename K, typename V>
inline std::pair<const K, V>& map_entry_to_value_type(std::pair<K, V>& p) {
    return *map_entry_to_value_type_ptr(&p);
}

inline constexpr usize ceil_next_power_of_2(usize k) {
    const usize l = log2ui(k);
    return (k == (1_uu << l)) ? k : (2_uu << l);
}


static constexpr double default_hash_map_max_load_factor = 2.0 / 3.0;

enum class ProbingStrategy {
    Linear,
    Quadratic
};

static constexpr ProbingStrategy default_hash_map_probing_strategy = ProbingStrategy::Linear;

// http://research.cs.vt.edu/AVresearch/hashing/quadratic.php
template<ProbingStrategy Strategy = default_hash_map_probing_strategy>
inline constexpr usize probing_offset(usize i) {
    if constexpr(Strategy == ProbingStrategy::Linear) {
        return i;
    } else {
        return (i * i + i) / 2;
    }
}
}


namespace swiss {
template<typename Key, typename Value, typename Hasher = Hash<Key>, typename Equal = std::equal_to<Key>>
class FlatHashMap : Hasher, Equal {
    public:
        using key_type = std::remove_cvref_t<Key>;
        using mapped_type = std::remove_cvref_t<Value>;
        using value_type = std::pair<const key_type, mapped_type>;

        static constexpr double max_load_factor = detail::default_hash_map_max_load_factor;
        static constexpr usize min_capacity = 16;

    private:
        using pair_type = std::pair<key_type, mapped_type>;

        static constexpr usize invalid_index = usize(-1);

        struct Bucket {
            usize index;
            usize hash;
        };

        using State = u8;
        static constexpr State tombstone_state  = 0x01;
        static constexpr State empty_state      = 0x00;
        static constexpr State state_has_hash_bit     = 0x80;

        static inline u8 make_state(usize hash) {
            return u8(hash & 0xFF) | state_has_hash_bit;
        }

        static inline bool is_state_full(State state) {
            return (state & state_has_hash_bit) != 0;
        }

        static inline bool matches_hash(State state, usize hash) {
            return state == make_state(hash);
        }

        template<typename K>
        inline usize retrieve_hash(const K& key, const State&) const {
            return hash(key);
        }

        struct Entry : NonMovable {
            union {
                pair_type key_value;
            };

            inline Entry() {
            }

            inline ~Entry() {
                checked_set_full();
            }

            template<typename K>
            inline void set_empty(const K& k) {
                checked_set_full();
                ::new(&key_value) pair_type{k, mapped_type{}};
            }

            inline void set(pair_type&& kv) {
                checked_set_full();
                ::new(&key_value) pair_type{std::move(kv)};
            }

            inline void clear() {
                checked_set_empty();
                key_value.~pair_type();
            }

            inline const key_type& key() const {
                y_debug_assert(!empty);
                return key_value.first;
            }

#ifdef Y_DEBUG
            bool empty = true;

            inline void checked_set_full() {
                y_debug_assert(empty);
                empty = false;
            }

            inline void checked_set_empty() {
                y_debug_assert(!empty);
                empty = true;
            }
#else
            inline void checked_set_full() {
            }

            inline void checked_set_empty() {
            }
#endif
        };

        struct KeyValueIt {
            using type = value_type;

            inline value_type& operator()(Entry& entry) const {
                return detail::map_entry_to_value_type(entry.key_value);
            }

            inline const value_type& operator()(const Entry& entry) const {
                return detail::map_entry_to_value_type(entry.key_value);
            }
        };

        struct KeyIt {
            using type = key_type;

            inline const key_type& operator()(const Entry& entry) const {
                return entry.key();
            }
        };

        struct ValueIt {
            using type = mapped_type;

            inline mapped_type& operator()(Entry& entry) const {
                return entry.key_value.second;
            }

            inline const mapped_type& operator()(const Entry& entry) const {
                return entry.key_value.second;
            }
        };

        template<bool Const, typename Transform>
        class IteratorBase : Transform {

            using parent_type = const_type_t<Const, FlatHashMap>;

            public:
                inline IteratorBase() = default;
                inline IteratorBase(const IteratorBase&) = default;
                inline IteratorBase& operator=(const IteratorBase&) = default;

                template<bool C, typename T> requires(Const > C)
                inline IteratorBase(const IteratorBase<C, T>& other) {
                    operator=(other);
                }

                template<bool C, typename T> requires(Const > C)
                inline IteratorBase& operator=(const IteratorBase<C, T>& other) {
                    _index = other._index;
                    _parent = other._parent;
                    return *this;
                }

                inline decltype(auto) operator*() const {
                    return Transform::operator()(_parent->_entries[_index]);
                }

                inline decltype(auto) operator->() const {
                    return &Transform::operator()(_parent->_entries[_index]);
                }

                inline IteratorBase& operator++() {
                    ++_index;
                    find_next();
                    return *this;
                }

                inline IteratorBase operator++(int) {
                    auto it = *this;
                    ++(*this);
                    return it;
                }

                inline bool at_end() const {
                    return _index == _parent->bucket_count();
                }

                template<bool C, typename T>
                inline bool operator==(const IteratorBase<C, T>& other) const {
                    return _index == other._index;
                }

                template<bool C, typename T>
                inline bool operator!=(const IteratorBase<C, T>& other) const {
                    return !operator==(other);
                }

            private:
                template<bool C, typename T>
                friend class IteratorBase;

                friend class FlatHashMap;

                inline IteratorBase(parent_type* parent, usize index) : _index(index), _parent(parent) {
                    find_next();
                }

                inline void find_next() {
                    for(;_index < _parent->bucket_count() && !is_state_full(_parent->_states[_index]); ++_index) {
                        // nothing
                    }
                    y_debug_assert(_index <= _parent->bucket_count());
                    y_debug_assert(at_end() || is_state_full(_parent->_states[_index]));
                }

                Y_TODO(Maybe dont store pointer to parent and use _state directly (starting from the end so we get _index -> 0))

                usize _index = invalid_index;
                parent_type* _parent = nullptr;

            public:
                using iterator_category = std::forward_iterator_tag;
                using difference_type = std::ptrdiff_t;

                using value_type = const_type_t<Const, typename Transform::type>;
                using reference = value_type&;
                using pointer = value_type*;
        };

        inline bool should_expand() const {
            return bucket_count() * max_load_factor <= _size;
        }

        template<typename K>
        inline usize hash(const K& key) const {
            return Hasher::operator()(key);
        }

        template<typename K>
        inline bool equal(const key_type& a, const K& b) const {
            return Equal::operator()(a, b);
        }

        template<typename K>
        inline Bucket find_bucket_for_insert(const K& key) {
            const usize h = hash(key);
            return find_bucket_for_insert(key, h);
        }

        template<typename K>
        Bucket find_bucket_for_insert(const K& key, usize h) {
            const usize buckets = bucket_count();
            const usize hash_mask = buckets - 1;
            usize probes = 0;

            y_debug_assert(buckets);
            {
                usize best_index = invalid_index;
                for(; probes <= _max_probe_len; ++probes) {
                    const usize index = (h + detail::probing_offset<>(probes)) & hash_mask;
                    const State& state = _states[index];
                    if(!is_state_full(state)) {
                        if(state == empty_state) {
                            return {index, h};
                        }
                        if(best_index == invalid_index) {
                            best_index = index;
                        }
                    } else if(matches_hash(state, h) && equal(_entries[index].key(), key)) {
                        return {index, h};
                    }
                }

                if(best_index != invalid_index) {
                    return {best_index, h};
                }
            }

            for(; probes < buckets; ++probes) {
                const usize index = (h + detail::probing_offset<>(probes)) & hash_mask;
                if(!is_state_full(_states[index])) {
                    _max_probe_len = probes;
                    return {index, h};
                }
            }

            y_fatal("Internal error: unable to find empty bucket");
        }

        template<typename K>
        usize find_bucket(const K& key) const {
            if(is_empty()) {
                return invalid_index;
            }

            const usize h = hash(key);
            const usize hash_mask = bucket_count() - 1;
            for(usize i = 0; i <= _max_probe_len; ++i) {
                const usize index = (h + detail::probing_offset<>(i)) & hash_mask;
                const State& state = _states[index];
                if(matches_hash(state, h)) {
                    if(equal(_entries[index].key(), key)) {
                        return index;
                    }
                } else if(state == empty_state) {
                    return invalid_index;
                }
            }
            return invalid_index;
        }

        void expand(usize new_bucket_count) {
            const usize pow_2 = detail::ceil_next_power_of_2(new_bucket_count);
            const usize new_size = pow_2 < min_capacity ? min_capacity : pow_2;

            y_debug_assert(pow_2 >= new_bucket_count);

            if(new_size <= bucket_count()) {
                return;
            }

            auto old_states = std::exchange(_states, FixedArray<State>(new_size));
            auto old_entries = std::exchange(_entries, std::make_unique_for_overwrite<Entry[]>(new_size));
            _max_probe_len = 0;

            if(_size) {
                const usize old_bucket_count = old_states.size();
                for(usize i = 0; i != old_bucket_count; ++i) {
                    if(is_state_full(old_states[i])) {
                        const usize h = retrieve_hash(old_entries[i].key(), old_states[i]);
                        const Bucket bucket = find_bucket_for_insert(old_entries[i].key(), h);
                        const usize new_index = bucket.index;

                        y_debug_assert(!is_state_full(_states[new_index]));

                        _states[new_index] = make_state(bucket.hash);
                        _entries[new_index].set(std::move(old_entries[i].key_value));

                        old_entries[i].clear();
                    }
                }
            }
        }

        inline void expand() {
            expand(bucket_count() == 0 ? min_capacity : 2 * bucket_count());
        }

        FixedArray<State> _states;
        std::unique_ptr<Entry[]> _entries;
        usize _size = 0;
        usize _max_probe_len = 0;

    public:
        using iterator          = IteratorBase<false, KeyValueIt>;
        using const_iterator    = IteratorBase<true,  KeyValueIt>;

        static_assert(std::is_copy_assignable_v<const_iterator>);
        static_assert(std::is_copy_constructible_v<const_iterator>);
        static_assert(std::is_constructible_v<const_iterator, iterator>);
        static_assert(!std::is_constructible_v<iterator, const_iterator>);

        inline FlatHashMap() {
        }

        inline FlatHashMap(FlatHashMap&& other) {
            swap(other);
        }

        inline FlatHashMap& operator=(FlatHashMap&& other) {
            swap(other);
            return *this;
        }

        inline void swap(FlatHashMap& other) {
            if(&other != this) {
                std::swap(_states, other._states);
                std::swap(_entries, other._entries);
                std::swap(_size, other._size);
                std::swap(_max_probe_len, other._max_probe_len);
            }
        }

        inline ~FlatHashMap() {
            make_empty();
        }

        inline void make_empty() {
            const usize len = bucket_count();
            for(usize i = 0; i != len && _size; ++i) {
                if(is_state_full(_states[i])) {
                    _states[i] = empty_state;
                    _entries[i].clear();
                    --_size;
                }
            }
            _max_probe_len = 0;

            y_debug_assert(_size == 0);
        }

        inline void clear() {
            make_empty();
            _states.clear();
            _entries = nullptr;
        }

        inline iterator begin() {
            return iterator(this, 0);
        }

        inline const_iterator begin() const {
            return const_iterator(this, 0);
        }

        inline iterator end() {
            return iterator(this, bucket_count());
        }

        inline const_iterator end() const {
            return const_iterator(this, bucket_count());
        }

        inline auto key_values() {
            return core::Range(begin(), end());
        }

        inline auto key_values() const {
            return core::Range(begin(), end());
        }

        inline auto keys() const {
            return core::Range(
                IteratorBase<true, KeyIt>(this, 0),
                IteratorBase<true, KeyIt>(this, bucket_count())
            );
        }

        inline auto values() {
            return core::Range(
                IteratorBase<false, ValueIt>(this, 0),
                IteratorBase<false, ValueIt>(this, bucket_count())
            );
        }

        inline auto values() const {
            return core::Range(
                IteratorBase<true, ValueIt>(this, 0),
                IteratorBase<true, ValueIt>(this, bucket_count())
            );
        }


        inline bool is_empty() const {
            return !_size;
        }

        inline usize bucket_count() const {
            return _states.size();
        }

        inline usize size() const {
            return _size;
        }

        static inline constexpr usize max_size() {
            return decltype(_states)::max_size();
        }

        inline double load_factor() const {
            const usize buckets = bucket_count();
            return buckets ? double(_size) / double(buckets) : 0.0;
        }

        template<typename K>
        inline bool contains(const K& key) const {
            return find_bucket(key) != invalid_index;
        }

        template<typename K>
        inline iterator find(const K& key) {
            const usize index = find_bucket(key);
            if(index != invalid_index) {
                return iterator(this, index);
            }
            return end();
        }

        template<typename K>
        inline const_iterator find(const K& key) const {
            const usize index = find_bucket(key);
            if(index != invalid_index) {
                return const_iterator(this, index);
            }
            return end();
        }

        inline void rehash() {
            expand(bucket_count());
        }

        inline void set_min_capacity(usize cap) {
            const usize capacity = usize(cap / (max_load_factor * 0.8));
            if(bucket_count() < capacity) {
                expand(capacity);
            }
        }

        inline void reserve(usize cap) {
            set_min_capacity(cap);
        }

        template<typename K>
        inline void erase(const K& key) {
            if(const auto it = find(key); it != end()) {
                erase(it);
            }

        }

        inline void erase(const iterator& it) {
            const usize index = it._index;

            y_debug_assert(index < bucket_count());
            y_debug_assert(it._parent == this);
            y_debug_assert(is_state_full(_states[index]));

            _entries[index].clear();
            _states[index] = tombstone_state;

            --_size;
        }

        template<typename K, typename... Args>
        inline std::pair<iterator, bool> emplace(const K& key, Args&&... args) {
            return insert(pair_type{key, mapped_type{y_fwd(args)...}});
        }

        inline std::pair<iterator, bool> insert(value_type p) {
            if(should_expand()) {
                expand();
            }

            y_debug_assert(!should_expand());

            const Bucket bucket = find_bucket_for_insert(p.first);
            const usize index = bucket.index;
            const bool exists = is_state_full(_states[index]);

            y_debug_assert(!exists || _size > 0);
            if(!exists) {
                _entries[index].set(std::move(p));
                _states[index] = make_state(bucket.hash);
                ++_size;
            }

            return {iterator(this, index), !exists};
        }

        template<typename It>
        inline void insert(It beg, It en) {
            Y_TODO(Reserve if possible)
            for(; beg != en; ++beg) {
                insert(*beg);
            }
        }

        template<typename K>
        inline mapped_type& operator[](const K& key) {
            if(should_expand()) {
                expand();
            }

            const Bucket bucket = find_bucket_for_insert(key);
            const usize index = bucket.index;
            const bool exists = is_state_full(_states[index]);

            if(!exists) {
                _entries[index].set_empty(key);
                _states[index] = make_state(bucket.hash);
                ++_size;
            }

            return _entries[index].key_value.second;
        }
};
}

using namespace swiss;

}
}

#endif // Y_CORE_HASHMAP_H

