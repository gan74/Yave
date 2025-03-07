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
#include "Vector.h"

#include <y/utils/hash.h>
#include <y/utils/traits.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <functional>
#include <memory>
#include <bit>

#ifdef Y_SSE
#define Y_HASHMAP_SIMD
#include <emmintrin.h>
#endif


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


static constexpr double default_hash_map_max_load_factor = 0.8;

enum class ProbingStrategy {
    Linear,
    Quadratic
};

// http://research.cs.vt.edu/AVresearch/hashing/quadratic.php
template<ProbingStrategy Strategy>
inline constexpr usize probing_offset(usize i) {
    if constexpr(Strategy == ProbingStrategy::Linear) {
        return i;
    } else {
        return (i * i + i) / 2;
    }
}

using State = u8;
using State4 = u32;

static constexpr inline State4 to_state_4(State state) {
    const State4 s = state;
    return (s << 24) | (s << 16) | (s << 8) | s;
}

static constexpr State empty_state              = 0x00;
static constexpr State tombstone_state          = 0x01;
    
static constexpr State4 tombstone_state_4       = to_state_4(tombstone_state);

static constexpr auto state_table = [] {
    std::array<State, 256> table;
    for(usize i = 0; i != table.size(); ++i) {
        table[i] = State((i % 253) + (tombstone_state + 1));
    }
    return table;
}();

static constexpr auto state_table_4 = [] {
    std::array<State4, 256> table;
    for(usize i = 0; i != table.size(); ++i) {
        table[i] = to_state_4(state_table[i]);
    }
    return table;
}();
}


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

        static constexpr detail::ProbingStrategy probing_strategy = detail::ProbingStrategy::Quadratic;

        static constexpr usize invalid_index = usize(-1);

#ifdef Y_HASHMAP_SIMD
        static constexpr usize simd_width = sizeof(__m128i);
        static_assert(min_capacity >= simd_width);
        static_assert(min_capacity % simd_width == 0);
#endif

        using State = detail::State;
        using State4 = detail::State4;

        static y_force_inline State make_state(usize hash) {
            return detail::state_table[hash % detail::state_table.size()];
        }

        static y_force_inline State4 make_state_4(usize hash) {
            return detail::state_table_4[hash % detail::state_table_4.size()];
        }

        static y_force_inline bool is_state_full(State state) {
            return (state != detail::empty_state) && (state != detail::tombstone_state);
        }

        template<typename K>
        y_force_inline usize retrieve_hash(const K& key, const State&) const {
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
            y_force_inline void set_empty(const K& k) {
                checked_set_full();
                ::new(&key_value) pair_type{k, mapped_type{}};
            }

            y_force_inline void set(pair_type&& kv) {
                checked_set_full();
                ::new(&key_value) pair_type{std::move(kv)};
            }

            y_force_inline void clear() {
                checked_set_empty();
                key_value.~pair_type();
            }

            y_force_inline const key_type& key() const {
                y_debug_assert(!empty);
                return key_value.first;
            }

#ifdef Y_DEBUG
            bool empty = true;

            y_force_inline void checked_set_full() {
                y_debug_assert(empty);
                empty = false;
            }

            y_force_inline void checked_set_empty() {
                y_debug_assert(!empty);
                empty = true;
            }
#else
            y_force_inline void checked_set_full() {
            }

            y_force_inline void checked_set_empty() {
            }
#endif
        };

        struct KeyValueIt {
            using type = value_type;

            y_force_inline value_type& operator()(Entry& entry) const {
                return detail::map_entry_to_value_type(entry.key_value);
            }

            y_force_inline const value_type& operator()(const Entry& entry) const {
                return detail::map_entry_to_value_type(entry.key_value);
            }
        };

        struct KeyIt {
            using type = key_type;

            y_force_inline const key_type& operator()(const Entry& entry) const {
                return entry.key();
            }
        };

        struct ValueIt {
            using type = mapped_type;

            y_force_inline mapped_type& operator()(Entry& entry) const {
                return entry.key_value.second;
            }

            y_force_inline const mapped_type& operator()(const Entry& entry) const {
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

                y_force_inline void find_next() {
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
        y_force_inline usize hash(const K& key) const {
            return Hasher::operator()(key);
        }

        template<typename K>
        y_force_inline bool equal(const key_type& a, const K& b) const {
            return Equal::operator()(a, b);
        }

#ifdef Y_HASHMAP_SIMD
        static y_force_inline usize build_group_index(usize h, usize probe, usize mask) {
            return ((h >> 8) + detail::probing_offset<probing_strategy>(probe)) & mask;
        }

        static y_force_inline usize countr_zero(int x) {
            y_assume(x != 0);
#ifdef Y_MSVC
          unsigned long r;
          _BitScanForward(&r, unsigned(x));
          return r;
#else
          return std::countr_zero(unsigned(x));
#endif
        }


        template<typename K>
        y_force_inline std::pair<usize, bool> find_bucket_for_insert(const K& key, usize h) {
            const usize groups = group_count();
            const usize group_mask = groups - 1;
            
            const __m128i pattern = _mm_set1_epi32(make_state_4(h));
            const __m128i* gr = reinterpret_cast<const __m128i*>(_states.get());

            usize probes = 0;
            usize best_index = invalid_index;
            do {
                const usize group_index = build_group_index(h, probes, group_mask);
                const usize group_start_index = group_index * simd_width;
                const __m128i packed_states = _mm_loadu_si128(gr + group_index);

                {
                    int matches = _mm_movemask_epi8(_mm_cmpeq_epi8(packed_states, pattern));
                    if(matches != 0) {
                        do {
                            const Entry* group_start = _entries.get() + group_start_index;
                            _mm_prefetch(reinterpret_cast<const char*>(group_start), _MM_HINT_T0);
                            const usize index_in_group = countr_zero(matches);
                            if(equal(group_start[index_in_group].key(), key)) {
                                return {group_start_index + index_in_group, true};
                            }
                            matches ^= (matches & -matches);
                        } while(matches != 0);
                    }
                }

                if(best_index == invalid_index) {
                    const int matches = _mm_movemask_epi8(_mm_cmpeq_epi8(packed_states, _mm_set1_epi32(detail::tombstone_state_4)));
                    if(matches != 0) {
                        best_index = group_start_index + countr_zero(matches);
                    }
                }

                {
                    const int matches = _mm_movemask_epi8(_mm_cmpeq_epi8(packed_states, _mm_setzero_si128()));
                    if(matches != 0) {
                        if(best_index != invalid_index) {
                            return {best_index, false};
                        }
                        return {group_start_index + countr_zero(matches), false};
                    }
                }
            } while(++probes <= _max_probe_len);

            if(best_index != invalid_index) {
                return {best_index, false};
            }

            for(; probes < groups; ++probes) {
                const usize group_index = build_group_index(h, probes, group_mask);
                const usize group_start_index = group_index * simd_width;
                const __m128i packed_states = _mm_loadu_si128(gr + group_index);

                {
                    const int matches = _mm_movemask_epi8(_mm_cmpeq_epi8(packed_states, _mm_set1_epi32(detail::tombstone_state_4)));
                    if(matches != 0) {
                        _max_probe_len = probes;
                        const usize index = group_start_index + countr_zero(matches);
                        return {index, false};
                    }
                }
                
                {
                    const int matches = _mm_movemask_epi8(_mm_cmpeq_epi8(packed_states, _mm_setzero_si128()));
                    if(matches != 0) {
                        _max_probe_len = probes;
                        const usize index = group_start_index + countr_zero(matches);
                        return {index, false};
                    }
                }
            }

            y_unreachable();
        }

        template<typename K>
        y_force_inline usize find_bucket(const K& key) const {
            if(is_empty()) [[unlikely]] {
                return invalid_index;
            }

            const usize h = hash(key);

            const usize groups = group_count();
            const usize group_mask = groups - 1;

            y_debug_assert(groups);

            const __m128i pattern = _mm_set1_epi32(make_state_4(h));
            const __m128i* gr = reinterpret_cast<const __m128i*>(_states.get());

            usize probe = 0;
            do {
                const usize group_index = build_group_index(h, probe, group_mask);
                const __m128i packed_states = _mm_loadu_si128(gr + group_index);

                int matches = _mm_movemask_epi8(_mm_cmpeq_epi8(packed_states, pattern));
                if(matches != 0) [[likely]] {
                    const usize group_start_index = group_index * simd_width;
                    const Entry* group_start = _entries.get() + group_start_index;
                    _mm_prefetch(reinterpret_cast<const char*>(group_start), _MM_HINT_T0);
                    do {
                        const usize index_in_group = countr_zero(matches);
                        if(equal(group_start[index_in_group].key(), key)) [[likely]] {
                            return group_start_index + index_in_group;
                        }
                        matches ^= (matches & -matches);
                    } while(matches != 0);
                }

                if(_mm_movemask_epi8(_mm_cmpeq_epi8(packed_states, _mm_setzero_si128()))) [[likely]] {
                    return invalid_index;
                }

            } while(++probe <= _max_probe_len);

            return invalid_index;
        }
#else
        template<typename K>
        y_force_inline std::pair<usize, bool> find_bucket_for_insert(const K& key, usize h) {
            const usize buckets = bucket_count();
            const usize hash_mask = buckets - 1;

            y_debug_assert(buckets);

            usize probes = 0;
            usize best_index = invalid_index;
            do
                const usize index = (h + detail::probing_offset<probing_strategy>(probes)) & hash_mask;
                const State& state = _states[index];
                if(!is_state_full(state)) {
                    if(state == detail::empty_state) {
                        return {index, false};
                    }
                    if(best_index == invalid_index) {
                        best_index = index;
                    }
                } else if(state == make_state(h) && equal(_entries[index].key(), key)) {
                    return {index, true};
                }
            } while(++probes <= _max_probe_len);

            if(best_index != invalid_index) {
                return {best_index, false};
            }

            for(; probes < buckets; ++probes) {
                const usize index = (h + detail::probing_offset<probing_strategy>(probes)) & hash_mask;
                if(!is_state_full(_states[index])) {
                    _max_probe_len = probes;
                    return {index, false};
                }
            }

            y_unreachable();
        }

        template<typename K>
        usize find_bucket(const K& key) const {
            if(is_empty()) {
                return invalid_index;
            }

            const usize h = hash(key);
            const usize hash_mask = bucket_count() - 1;
            const State expected_state = make_state(h);

            for(usize i = 0; i <= _max_probe_len; ++i) {
                const usize index = (h + detail::probing_offset<probing_strategy>(i)) & hash_mask;
                const State& state = _states[index];
                if(state == expected_state) {
                    if(equal(_entries[index].key(), key)) {
                        return index;
                    }
                } else if(state == detail::empty_state) {
                    return invalid_index;
                }
            }
            return invalid_index;
        }
#endif

        void expand(usize cap) {
            y_debug_assert(is_pow_of_2(cap));

            const usize new_bucket_count = cap < min_capacity ? min_capacity : cap;
            if(new_bucket_count <= bucket_count()) {
                return;
            }

            const usize old_bucket_count = std::exchange(_buckets, new_bucket_count);
            auto old_states = std::exchange(_states, std::make_unique<State[]>(new_bucket_count));
            auto old_entries = std::exchange(_entries, std::make_unique_for_overwrite<Entry[]>(new_bucket_count));
            _max_probe_len = 0;

            if(_size) {
                for(usize i = 0; i != old_bucket_count; ++i) {
                    if(is_state_full(old_states[i])) {
                        const usize h = retrieve_hash(old_entries[i].key(), old_states[i]);
                        const auto [new_index, exists] = find_bucket_for_insert(old_entries[i].key(), h);
                        
                        y_debug_assert(!exists);
                        y_debug_assert(!is_state_full(_states[new_index]));

                        _states[new_index] = make_state(h);
                        _entries[new_index].set(std::move(old_entries[i].key_value));

                        old_entries[i].clear();
                    }
                }
            }
        }

        inline void expand() {
            expand(2 * bucket_count());
        }

        std::unique_ptr<State[]> _states;
        std::unique_ptr<Entry[]> _entries;
        usize _buckets = 0;
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
                std::swap(_buckets, other._buckets);
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
                    _states[i] = detail::empty_state;
                    _entries[i].clear();
                    --_size;
                }
            }
            _max_probe_len = 0;

            y_debug_assert(_size == 0);
        }

        inline void clear() {
            make_empty();
            _states = nullptr;
            _entries = nullptr;
            _buckets = 0;
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

        y_force_inline usize bucket_count() const {
            return _buckets;
        }
        
#ifdef Y_HASHMAP_SIMD
        y_force_inline usize group_count() const {
            return _buckets / simd_width;
        }
#endif

        inline usize max_probe_sequence_len() const {
            return _max_probe_len;
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

        inline void set_min_capacity(usize cap) {
            const usize capacity = next_pow_of_2(usize(cap / (max_load_factor * 0.8)));
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
            _states[index] = detail::tombstone_state;

            --_size;
        }

        template<typename K, typename... Args>
        inline std::pair<iterator, bool> emplace(const K& key, Args&&... args) {
            return insert(pair_type{key, mapped_type{y_fwd(args)...}});
        }

        inline std::pair<iterator, bool> insert(value_type p) {
            if(should_expand()) [[unlikely]] {
                expand();
            }

            y_debug_assert(!should_expand());
            
            const usize h = hash(p.first);
            const auto [index, exists] = find_bucket_for_insert(p.first, h);

            y_debug_assert(!exists || _size > 0);

            if(!exists) {
                _entries[index].set(std::move(p));
                _states[index] = make_state(h);
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
            if(should_expand()) [[unlikely]] {
                expand();
            }

            y_debug_assert(!should_expand());
            
            const usize h = hash(key);
            const auto [index, exists] = find_bucket_for_insert(key, h);

            if(!exists) {
                _entries[index].set_empty(key);
                _states[index] = make_state(h);
                ++_size;
            }

            return _entries[index].key_value.second;
        }
};


// Very slow
#if 0
template<typename Key, typename Value, typename Hasher = Hash<Key>, typename Equal = std::equal_to<Key>>
class DenseHashMap : Hasher, Equal {
    public:
        using key_type = std::remove_cvref_t<Key>;
        using mapped_type = std::remove_cvref_t<Value>;
        using value_type = std::pair<const key_type, mapped_type>;

        static constexpr double max_load_factor = detail::default_hash_map_max_load_factor;
        static constexpr usize min_capacity = 16;
        
        
        using iterator          = value_type*;
        using const_iterator    = const value_type*;

    private:
        using pair_type = std::pair<key_type, mapped_type>;
        
        static constexpr detail::ProbingStrategy probing_strategy = detail::ProbingStrategy::Linear;
        
        static constexpr u32 invalid_index = u32(-1);

        template<typename K>
        y_force_inline bool equal(const key_type& a, const K& b) const {
            return Equal::operator()(a, b);
        }
        
        template<typename K>
        y_force_inline usize hash(const K& key) const {
            return Hasher::operator()(key);
        }

        y_force_inline u32 reduce_hash(usize h) const {
            return u32(h >> ((sizeof(usize) - sizeof(u32)) * 8));
        }

        y_force_inline usize bucket_hash_index(usize index) const {
            return index * 2;
        }

        y_force_inline usize bucket_index_index(usize index) const {
            return index * 2 + 1;
        }
        
    public:
        inline DenseHashMap() {
        }

        inline DenseHashMap(DenseHashMap&& other) {
            swap(other);
        }

        inline DenseHashMap& operator=(DenseHashMap&& other) {
            swap(other);
            return *this;
        }

        inline void swap(DenseHashMap& other) {
            if(&other != this) {
                _key_values.swap(other._key_values);
                std::swap(_metadata, other._metadata);
                std::swap(_bucket_count, other._bucket_count);
                std::swap(_max_probe_len, other._max_probe_len);
            }
        }

        inline ~DenseHashMap() {
            clear();
        }

        inline void make_empty() {
            _key_values.make_empty();
            std::fill_n(_metadata.get(), _bucket_count, invalid_index);
            _max_probe_len = 0;
        }

        inline void clear() {
            _key_values.clear();
            _metadata = nullptr;
            _bucket_count = 0;
            _max_probe_len = 0;
        }
        
        inline usize size() const {
            return _key_values.size();
        }
        
        inline bool is_empty() const {
            return _key_values.is_empty();
        }
        
        inline usize bucket_count() const {
            return _bucket_count;
        }
        
        inline usize max_probe_sequence_len() const {
            return _max_probe_len;
        }
        
        inline iterator begin() {
            return _key_values.begin();
        }
        
        inline iterator end() {
            return _key_values.end();
        }
        
        inline const_iterator begin() const {
            return _key_values.begin();
        }
        
        inline const_iterator end() const {
            return _key_values.end();
        }
        
        inline void set_min_capacity(usize cap) {
            const usize capacity = next_pow_of_2(usize(cap / (max_load_factor * 0.8)));
            if(_bucket_count < capacity) {
                expand(capacity);
            }
        }

        inline void reserve(usize cap) {
            set_min_capacity(cap);
        }
    
        template<typename K>
        inline iterator find(const K& key) {
            const u32 index = find_index(key);
            return index == invalid_index ? end() : (_key_values.begin() + index);
        }

        template<typename K>
        inline const_iterator find(const K& key) const {
            const u32 index = find_index(key);
            return index == invalid_index ? end() : (_key_values.begin() + index);
        }
        
        template<typename K>
        inline bool contains(const K& key) const {
            return find_index(key) != invalid_index;
        }
        
        inline std::pair<iterator, bool> insert(value_type p) {
            if(should_expand()) {
                expand();
            }
            
            const u32 index = find_index_for_insert(p.first, u32(_key_values.size()));
            if(index != invalid_index) {
                return {_key_values.begin() + index, true};
            }
            
            _key_values.emplace_back(std::move(p));
            return {_key_values.end() - 1, false};
        }
        
        template<typename K>
        inline mapped_type& operator[](const K& key) {
            if(should_expand()) {
                expand();
            }
            
            const u32 index = find_index_for_insert(key, u32(_key_values.size()));
            if(index != invalid_index) {
                return _key_values[index].second;
            }
            
            return _key_values.emplace_back(value_type(key, mapped_type{})).second;
        }
        
    private:
        template<typename K>
        inline u32 find_index_for_insert(const K& key, u32 insert_index) {
            const usize h = hash(key);
            const u32 reduced_hash = reduce_hash(h);
            
            y_debug_assert(is_pow_of_2(_bucket_count));
            const usize mask = _bucket_count - 1;
            
            usize probes = 0;
            do {
                const usize index = (h + detail::probing_offset<probing_strategy>(probes)) & mask;
                
                u32& bucket_index = _metadata[bucket_index_index(index)];
                u32& bucket_hash = _metadata[bucket_hash_index(index)];
                if(bucket_index == invalid_index) {
                    bucket_index = insert_index;
                    bucket_hash = reduced_hash;
                    return invalid_index;
                }
                
                if(bucket_hash == reduced_hash) {
                    if(equal(_key_values[bucket_index].first, key)) {
                        return bucket_index;
                    }
                }
            } while(++probes <= _max_probe_len);
            
            for(; probes != _bucket_count; ++probes) {
                const usize index = (h + detail::probing_offset<probing_strategy>(probes)) & mask;
                u32& bucket_index = _metadata[bucket_index_index(index)];
                if(bucket_index == invalid_index) {
                    bucket_index = insert_index;
                    _metadata[bucket_hash_index(index)] = reduced_hash;
                    _max_probe_len = probes;
                    return invalid_index;
                }
            }
            
            y_unreachable();
        }
        
        template<typename K>
        inline u32 find_index(const K& key) const {
            const usize h = hash(key);
            const u32 reduced_hash = reduce_hash(h);
            
            y_debug_assert(is_pow_of_2(_bucket_count));
            const usize mask = _bucket_count - 1;
            
            for(usize i = 0; i <= _max_probe_len; ++i) {
                const usize index = (h + detail::probing_offset<probing_strategy>(i)) & mask;
                
                const u32 bucket_index = _metadata[bucket_index_index(index)];
                if(bucket_index == invalid_index) {
                    return invalid_index;
                }
                
                const u32 bucket_hash = _metadata[bucket_hash_index(index)];
                if(bucket_hash == reduced_hash) {
                    if(equal(_key_values[bucket_index].first, key)) {
                        return bucket_index;
                    }
                }
            }
            
            return invalid_index;
        }
        
        inline bool should_expand() const {
            return _bucket_count * max_load_factor <= size();
        }
        
        inline void expand() {
            expand(2 * _bucket_count);
        }
        
        void expand(usize cap) {
            y_debug_assert(is_pow_of_2(cap));

            const usize new_bucket_count = cap < min_capacity ? min_capacity : cap;
            if(new_bucket_count <= _bucket_count) {
                return;
            }

            _bucket_count = new_bucket_count;

            _key_values.set_min_capacity(usize(_bucket_count / max_load_factor) + 1);

            _metadata = std::make_unique_for_overwrite<u32[]>(_bucket_count * 2);
            std::fill_n(_metadata.get(), _bucket_count * 2, invalid_index);

            for(usize i = 0; i != _key_values.size(); ++i) {
                [[maybe_unused]] const u32 index = find_index_for_insert(_key_values[i].first, u32(i));
                y_debug_assert(index == invalid_index);
            }
        }
        
        
        core::Vector<value_type> _key_values;
        std::unique_ptr<u32[]> _metadata;
        usize _bucket_count = 0;
        usize _max_probe_len = 0;
};
#endif

}
}

#endif // Y_CORE_HASHMAP_H

