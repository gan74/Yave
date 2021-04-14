/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include "Vector.h"
#include "Range.h"
#include "FixedArray.h"

#include <y/utils/traits.h>

// #define Y_HASHMAP_AUDIT

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
    return 1_uu << log2ui(k + 1);
}


static constexpr double default_hash_map_max_load_factor = 2.0 / 3.0;

enum class ProbingStrategy {
    Linear,
    Quadratic
};

static constexpr ProbingStrategy default_hash_map_probing_strategy = ProbingStrategy::Quadratic;

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


namespace external {
template<typename Key, typename Value, typename Hasher = std::hash<Key>, bool StoreHash = false>
class ExternalHashMap : Hasher {
    public:
        using key_type = remove_cvref_t<Key>;
        using mapped_type = remove_cvref_t<Value>;
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

        struct StateHash {
            static constexpr usize hash_bit = usize(1) << (8 * sizeof(usize) - 1);
            static constexpr usize empty_states = 0;
            static constexpr usize tombstone_states = 1;

            usize bits = empty_states;

            inline void set_hash(usize hash) {
                y_debug_assert(!is_full());
                bits = hash | hash_bit;
            }

            inline void make_empty() {
                y_debug_assert(is_full());
                bits = tombstone_states;
            }

            inline bool is_full() const {
                return bits & hash_bit;
            }

            inline bool is_hash(usize hash) const {
                return bits == (hash | hash_bit);
            }

            inline bool is_empty_strict() const {
                return bits == empty_states;
            }

            inline bool is_tombstone() const {
                return bits == tombstone_states;
            }

            inline usize hash() const {
                y_debug_assert(is_full());
                // we dont care about the last bit since we mask instead of %
                return bits;
            }
        };

        struct SimpleState {
            enum State : u8 {
                Empty,
                Full,
                Tombstone
            } state = Empty;

            inline void set_hash(usize) {
                state = Full;
            }

            inline void make_empty() {
                y_debug_assert(is_full());
                state = Tombstone;
            }

            inline bool is_full() const {
                return state == Full;
            }

            inline bool is_hash(usize) const {
                return is_full();
            }

            inline bool is_empty_strict() const {
                return state == Empty;
            }

            inline bool is_tombstone() const {
                return state == Tombstone;
            }
        };

        static_assert(sizeof(StateHash) == sizeof(usize));
        static_assert(sizeof(SimpleState) == sizeof(u8));

        inline usize retrieve_hash(const key_type&, const StateHash& state) const {
            return state.hash();
        }

        inline usize retrieve_hash(const key_type& key, const SimpleState&) const {
            return hash(key);
        }


        using State = std::conditional_t<StoreHash, StateHash, SimpleState>;

        struct Entry : NonMovable {
            union {
                pair_type key_value;
            };

            inline Entry() {
            }

            inline ~Entry() {
            }


            inline void set_empty(const key_type& k) {
                ::new(&key_value) pair_type{k, mapped_type{}};
            }

            inline void set(pair_type&& kv) {
                ::new(&key_value) pair_type{std::move(kv)};
            }

            inline void clear() {
                key_value.~pair_type();
            }

            inline const key_type& key() const {
                return key_value.first;
            }
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

            using parent_type = const_type_t<Const, ExternalHashMap>;

            public:
                inline IteratorBase() = default;
                inline IteratorBase(const IteratorBase&) = default;
                inline IteratorBase& operator=(const IteratorBase&) = default;

                template<bool C, typename T, typename = std::enable_if_t<(Const > C)>>
                inline IteratorBase(const IteratorBase<C, T>& other) {
                    operator=(other);
                }

                template<bool C, typename T, typename = std::enable_if_t<(Const > C)>>
                inline IteratorBase& operator=(const IteratorBase<C, T>& other) {
                    _index = other._index;
                    _parent = other._parent;
                    return *this;
                }

                inline auto& operator*() const {
                    return Transform::operator()(_parent->_entries[_index]);
                }

                inline auto* operator->() const {
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

                friend class ExternalHashMap;

                inline IteratorBase(parent_type* parent, usize index) : _index(index), _parent(parent) {
                    find_next();
                }

                inline void find_next() {
                    for(;_index < _parent->bucket_count() && !_parent->_states[_index].is_full(); ++_index) {
                        // nothing
                    }
                    y_debug_assert(_index <= _parent->bucket_count());
                    y_debug_assert(at_end() || _parent->_states[_index].is_full());
                }

                Y_TODO(Maybe dont store pointer to parent and use _state directly (starting from the end so we get _index -> 0))

                usize _index = invalid_index;
                parent_type* _parent = nullptr;

            public:
                using iterator_category = std::forward_iterator_tag;
                using difference_type = usize;

                using value_type = const_type_t<Const, typename Transform::type>;
                using reference = value_type&;
                using pointer = value_type*;
        };

        inline bool should_expand() const {
            return bucket_count() * max_load_factor <= _size;
        }

        inline usize hash(const key_type& key) const {
            return Hasher::operator()(key);
        }

        inline Bucket find_bucket_for_insert(const key_type& key) {
            const usize h = hash(key);
            return find_bucket_for_insert(key, h);
        }

        Bucket find_bucket_for_insert(const key_type& key, usize h) {
            const usize buckets = bucket_count();
            const usize hash_mask = buckets - 1;
            usize probes = 0;

            y_debug_assert(buckets);
            {
                usize best_index = invalid_index;
                for(; probes <= _max_probe_len; ++probes) {
                    const usize index = (h + detail::probing_offset<>(probes)) & hash_mask;
                    const State& state = _states[index];
                    if(!state.is_full()) {
                        best_index = index;
                        if(state.is_empty_strict()) {
                            return {best_index, h};
                        }
                    } else if(state.is_hash(h) && _entries[index].key() == key) {
                        return {index, h};
                    }
                }

                if(best_index != invalid_index) {
                    return {best_index, h};
                }
            }

            for(; probes < buckets; ++probes) {
                const usize index = (h + detail::probing_offset<>(probes)) & hash_mask;
                if(!_states[index].is_full()) {
                    _max_probe_len = probes;
                    return {index, h};
                }
            }

            y_fatal("Internal error: unable to find empty bucket");
        }

        usize find_bucket(const key_type& key) const {
            if(is_empty()) {
                return invalid_index;
            }

            const usize h = hash(key);
            const usize hash_mask = bucket_count() - 1;
            for(usize i = 0; i <= _max_probe_len; ++i) {
                const usize index = (h + detail::probing_offset<>(i)) & hash_mask;
                const State& state = _states[index];
                if(state.is_hash(h)) {
                    if(_entries[index].key() == key) {
                        return index;
                    }
                } else if(state.is_empty_strict()) {
                    return invalid_index;
                }
            }
            return invalid_index;
        }

        void expand(usize new_bucket_count) {
            const usize pow_2 = detail::ceil_next_power_of_2(new_bucket_count);
            const usize new_size = pow_2 < min_capacity ? min_capacity : pow_2;

            if(new_size <= bucket_count()) {
                return;
            }

            auto old_states = std::exchange(_states, FixedArray<State>(new_size));
            auto old_entries = std::exchange(_entries, std::make_unique<Entry[]>(new_size));
            _max_probe_len = 0;

            if(_size) {
                const usize old_bucket_count = old_states.size();
                for(usize i = 0; i != old_bucket_count; ++i) {
                    if(old_states[i].is_full()) {
                        const usize h = retrieve_hash(old_entries[i].key(), old_states[i]);
                        const Bucket bucket = find_bucket_for_insert(old_entries[i].key(), h);
                        const usize new_index = bucket.index;

                        y_debug_assert(!_states[new_index].is_full());

                        _states[new_index].set_hash(bucket.hash);
                        _entries[new_index].set(std::move(old_entries[i].key_value));
                    }
                }
            }

            audit();
        }

        inline void expand() {
            expand(bucket_count() == 0 ? min_capacity : 2 * bucket_count());
        }

        void audit() {
#ifdef Y_HASHMAP_AUDIT
            usize entry_count = 0;
            for(usize i = 0; i != bucket_count(); ++i) {
                if(!_states[i].is_full()) {
                    continue;
                }
                ++entry_count;
                const key_type& k = _entries[i].key();
                const Bucket b = find_bucket_for_insert(k);
                y_debug_assert(b.index == i);
            }
            y_debug_assert(entry_count == _size);
#endif
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

        inline ExternalHashMap() {
            audit();
        }

        inline ExternalHashMap(ExternalHashMap&& other) {
            swap(other);
        }

        inline ExternalHashMap& operator=(ExternalHashMap&& other) {
            swap(other);
            return *this;
        }

        inline void swap(ExternalHashMap& other) {
            if(&other != this) {
                std::swap(_states, other._states);
                std::swap(_entries, other._entries);
                std::swap(_size, other._size);
                std::swap(_max_probe_len, other._max_probe_len);
            }
            audit();
        }

        inline ~ExternalHashMap() {
            make_empty();
        }

        inline void make_empty() {
            const usize len = bucket_count();
            for(usize i = 0; i != len && _size; ++i) {
                if(_states[i].is_full()) {
                    _states[i] = State();
                    _entries[i].clear();
                    --_size;
                }
            }
            _max_probe_len = 0;

            y_debug_assert(_size == 0);
            audit();
        }

        inline void clear() {
            make_empty();
            _states.clear();
            _entries = nullptr;
            audit();
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

        inline double load_factor() const {
            return double(_size) / double(_entries.size());
        }

        inline bool contains(const key_type& key) const {
            return find_bucket(key) != invalid_index;
        }

        inline iterator find(const key_type& key) {
            const usize index = find_bucket(key);
            if(index != invalid_index) {
                return iterator(this, index);
            }
            return end();
        }

        inline const_iterator find(const key_type& key) const {
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

        inline void erase(const iterator& it) {
            y_defer(audit());

            const usize index = it._index;

            y_debug_assert(index < bucket_count());
            y_debug_assert(it._parent == this);

            _entries[index].clear();
            _states[index].make_empty();

            --_size;
        }

        template<typename... Args>
        inline std::pair<iterator, bool> emplace(const key_type& key, Args&&... args) {
            return insert(pair_type{key, mapped_type{y_fwd(args)...}});
        }

        inline std::pair<iterator, bool> insert(pair_type p) {
            y_defer(audit());

            if(should_expand()) {
                expand();
            }

            y_debug_assert(!should_expand());

            const Bucket bucket = find_bucket_for_insert(p.first);
            const usize index = bucket.index;
            const bool exists = _states[index].is_full();

            y_debug_assert(!exists || _size > 0);
            if(!exists) {
                _entries[index].set(std::move(p));
                _states[index].set_hash(bucket.hash);
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

        inline mapped_type& operator[](const key_type& key) {
            if(should_expand()) {
                expand();
            }

            const Bucket bucket = find_bucket_for_insert(key);
            const usize index = bucket.index;
            const bool exists = _states[index].is_full();

            if(!exists) {
                _entries[index].set_empty(key);
                _states[index].set_hash(bucket.hash);
                ++_size;
            }

            return _entries[index].key_value.second;
        }
};
}

using namespace external;

}
}

#endif // Y_CORE_HASHMAP_H

