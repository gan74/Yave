/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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
#ifndef Y_CORE_DENSEMAP_H
#define Y_CORE_DENSEMAP_H

#include "Vector.h"
#include "FixedArray.h"

#include <y/utils/iter.h>
#include <y/utils/traits.h>


//#define Y_DENSEMAP_AUDIT

namespace y {
namespace core {

// http://research.cs.vt.edu/AVresearch/hashing/index.php

Y_TODO(Performance degrades very heavily when erasing)
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
	return 1 << log2ui(k + 1);
}


static constexpr double default_dense_map_max_load_factor = 2.0 / 3.0;

enum class ProbingStrategy {
	Linear,
	Quadratic
};

static constexpr ProbingStrategy default_dense_map_probing_strategy = ProbingStrategy::Quadratic;

// http://research.cs.vt.edu/AVresearch/hashing/quadratic.php
template<ProbingStrategy Strategy = default_dense_map_probing_strategy>
inline constexpr usize probing_offset(usize i) {
	if constexpr(Strategy == ProbingStrategy::Linear) {
		return i;
	} else {
		return (i * i + i) / 2;
	}
}
}


namespace compact {
template<typename Key, typename Value, typename Hasher = std::hash<Key>>
class DenseMap : Hasher {
	public:
		using key_type = remove_cvref_t<Key>;
		using mapped_type = remove_cvref_t<Value>;
		using value_type = std::pair<const key_type, mapped_type>;

	private:
		using pair_type = std::pair<key_type, mapped_type>;


		enum class EntryState : u8 {
			Empty = 0,
			Tombstone = 1,
			Full = 2
		};

		struct Entry : NonCopyable {
			union {
				pair_type key_value;
			};

			EntryState state = EntryState::Empty;


			Entry() {
			}

			~Entry() {
				make_empty();
			}

			Entry(Entry&& other) {
				operator=(std::move(other));
			}

			Entry& operator=(Entry&& other) {
				make_empty();
				if(other.is_full()) {
					state = EntryState::Full;
					::new(&key_value) pair_type{std::move(other.key_value)};
				}
				return *this;
			}

			bool is_full() const {
				return state == EntryState::Full;
			}

			bool is_empty_strict() const {
				return state == EntryState::Empty;
			}

			bool is_tombstone() const {
				return state == EntryState::Tombstone;
			}

			void emplace(pair_type&& kv) {
				y_debug_assert(!is_full());
				state = EntryState::Full;
				::new(&key_value) pair_type{std::move(kv)};
			}

			void make_empty() {
				if(is_full()) {
					state = EntryState::Tombstone;
					key_value.~pair_type();
				}
			}

			const key_type& key() const {
				y_debug_assert(is_full());
				return key_value.first;
			}
		};

	public:
		static constexpr double max_load_factor = detail::default_dense_map_max_load_factor;
		static constexpr usize min_capacity = 16;

		auto key_values() const {
			return core::Range(
				make_iterator(_entries.begin()),
				EndIterator{}
			);
		}

		auto key_values() {
			return core::Range(
				make_iterator(_entries.begin()),
				EndIterator{}
			);
		}

		auto values() const {
			return core::Range(
				TransformIterator(entry_iterator(_entries.begin()), [](const Entry& entry) -> const mapped_type& {
					return entry.key_value.second;
				}),
				EndIterator{}
			);
		}

		auto values() {
			return core::Range(
				TransformIterator(entry_iterator(_entries.begin()), [](Entry& entry) -> mapped_type& {
					return entry.key_value.second;
				}),
				EndIterator{}
			);
		}

		auto keys() const {
			return core::Range(
				TransformIterator(entry_iterator(_entries.begin()), [](const Entry& entry) -> const key_type& {
					return entry.key();
				}),
				EndIterator{}
			);
		}

	private:
		auto entry_iterator(const Entry* entry) const {
			return FilterIterator(entry, _entries.end(), [](const Entry& entry) { return entry.is_full(); });
		}

		auto entry_iterator(Entry* entry) {
			return FilterIterator(entry, _entries.end(), [](Entry& entry) { return entry.is_full(); });
		}

		auto make_iterator(const Entry* entry) const {
			return TransformIterator(entry_iterator(entry), [](const Entry& entry) -> const value_type& {
				return detail::map_entry_to_value_type(entry.key_value);
			});
		}

		auto make_iterator(Entry* entry) {
			return TransformIterator(entry_iterator(entry), [](Entry& entry) -> value_type& {
				return detail::map_entry_to_value_type(entry.key_value);
			});
		}


		bool should_expand() const {
			return _entries.size() * max_load_factor <= _size;
		}

		usize hash(const key_type& key) const {
			return Hasher::operator()(key);
		}

		usize find_bucket_index_for_insert(const key_type& key) {
			const usize h = hash(key);
			const usize entry_count = _entries.size();
			const usize hash_mask = entry_count - 1;
			usize probes = 0;

			{
				usize best_index = usize(-1);
				for(; probes <= _max_probe_len; ++probes) {
					const usize index = (h + detail::probing_offset<>(probes)) & hash_mask;
					const Entry& entry = _entries[index];
					if(!entry.is_full()) {
						best_index = index;
						if(entry.is_empty_strict()) {
							return best_index;
						}
					} else if(_entries[index].key() == key) {
						return index;
					}
				}

				if(best_index != usize(-1)) {
					return best_index;
				}
			}

			for(; probes != entry_count; ++probes) {
				const usize index = (h + detail::probing_offset<>(probes)) & hash_mask;
				if(!_entries[index].is_full()) {
					_max_probe_len = probes;
					return index;
				}
			}

			y_fatal("Internal error: unable to find empty bucket");
		}

		const Entry* find_entry(const key_type& key) const {
			const usize h = hash(key);
			const usize entry_count = _entries.size();
			const usize hash_mask = entry_count - 1;
			for(usize i = 0; i <= _max_probe_len; ++i) {
				const usize index = (h + detail::probing_offset<>(i)) & hash_mask;
				const Entry& entry = _entries[index];
				if(entry.is_full() && entry.key() == key) {
					return &entry;
				}
				if(entry.is_empty_strict()) {
					return nullptr;
				}
			}
			return nullptr;
		}

		void expand(usize new_entry_count) {
			const usize pow_2 = detail::ceil_next_power_of_2(new_entry_count);
			const usize new_size = pow_2 < min_capacity ? min_capacity : pow_2;

			if(new_size <= _entries.size()) {
				return;
			}

			auto old_entries = std::exchange(_entries, FixedArray<Entry>(new_size));
			for(Entry& k : old_entries) {
				if(k.is_full()) {
					const usize new_index = find_bucket_index_for_insert(k.key());
					y_debug_assert(!_entries[new_index].is_full());
					_entries[new_index].emplace(std::move(k.key_value));
				}
			}
		}

		void expand() {
			expand(_entries.size() == 0 ? min_capacity : 2 * _entries.size());
		}

		void audit() {
#ifdef Y_DENSEMAP_AUDIT
			y_debug_assert(_entries.size() >= _size + 1);
			usize entry_count = 0;
			for(usize i = 0; i != _entries.size(); ++i) {
				const Entry& k = _entries[i];
				if(!k.is_full()) {
					continue;
				}
				++entry_count;
				y_debug_assert(find_entry(k.key()) == &k);
			}
			y_debug_assert(_size == entry_count);
#endif
		}

		FixedArray<Entry> _entries;
		usize _size = 0;
		usize _max_probe_len = 0;

	public:
		using iterator			= typename decltype(std::declval<      DenseMap<Key, Value, Hasher>>().key_values())::iterator;
		using const_iterator	= typename decltype(std::declval<const DenseMap<Key, Value, Hasher>>().key_values())::iterator;

		static_assert(std::is_copy_assignable_v<const_iterator>);
		static_assert(std::is_copy_constructible_v<const_iterator>);

		DenseMap() = default;
		DenseMap(DenseMap&& other) {
			swap(other);
		}

		DenseMap& operator=(DenseMap&& other) {
			swap(other);
			return *this;
		}

		void swap(DenseMap& other) {
			if(&other != this) {
				std::swap(_entries, other._entries);
				std::swap(_size, other._size);
				std::swap(_max_probe_len, other._max_probe_len);
			}
		}

		iterator begin() {
			return key_values().begin();
		}

		const_iterator begin() const {
			return key_values().begin();
		}

		auto end() {
			return EndIterator{};
		}

		auto end() const {
			return EndIterator{};
		}


		usize size() const {
			return _size;
		}

		double load_factor() const {
			return double(_size) / double(_entries.size());
		}

		bool contains(const key_type& key) const {
			return find_entry(key);
		}

		iterator find(const key_type& key) {
			Entry* entry = const_cast<Entry*>(find_entry(key));
			return make_iterator(entry ? entry : _entries.end());
		}

		const_iterator find(const key_type& key) const {
			const Entry* entry = find_entry(key);
			return make_iterator(entry ? entry : _entries.end());
		}

		void rehash() {
			expand(_entries.size());
		}

		void set_min_capacity(usize cap) {
			const usize capacity = usize(cap / (max_load_factor * 0.8));
			if(_entries.size() < capacity) {
				expand(capacity);
			}
		}

		void reserve(usize cap) {
			set_min_capacity(cap);
		}

		template<typename... Args>
		std::pair<iterator, bool> emplace(const key_type& key, Args&&... args) {
			return insert(pair_type{key, mapped_type{y_fwd(args)...}});
		}

		std::pair<iterator, bool> insert(pair_type p) {
			y_defer(audit());

			if(should_expand()) {
				expand();
			}

			y_debug_assert(!should_expand());

			const usize index = find_bucket_index_for_insert(p.first);
			const bool exists = _entries[index].is_full();

			if(!exists) {
				_entries[index].emplace(std::move(p));
				++_size;
			}

			return {make_iterator(&_entries[index]), !exists};
		}

		void erase(const iterator& it) {
			y_defer(audit());

			Entry* entry_ptr = it.inner().inner();

			y_debug_assert(entry_ptr >= _entries.data());
			y_debug_assert(entry_ptr < _entries.data() + _entries.size());

			--_size;
			entry_ptr->make_empty();
		}
};
}

namespace bits {
template<typename Key, typename Value, typename Hasher = std::hash<Key>>
class ExternalBitsDenseMap : Hasher {
	public:
		using key_type = remove_cvref_t<Key>;
		using mapped_type = remove_cvref_t<Value>;
		using value_type = std::pair<const key_type, mapped_type>;

	private:
		using pair_type = std::pair<key_type, mapped_type>;

		struct Bucket {
			usize index;
			usize hash;
		};

		struct HashBits {
			static constexpr usize hash_bit = usize(1) << (8 * sizeof(usize) - 1);
			static constexpr usize empty_bits = 0;
			static constexpr usize tombstone_bits = 1;

			usize bits = empty_bits;

			void set_hash(usize hash) {
				y_debug_assert(!is_full());
				bits = hash | hash_bit;
			}

			void make_empty() {
				y_debug_assert(is_full());
				bits = tombstone_bits;
			}

			bool is_full() const {
				return bits & hash_bit;
			}

			bool is_hash(usize hash) const {
				return bits == (hash | hash_bit);
			}

			bool is_empty_strict() const {
				return bits == empty_bits;
			}

			bool is_tombstone() const {
				return bits == tombstone_bits;
			}

			usize hash() {
				y_debug_assert(is_full());
				return bits;
			}
		};

		static_assert(sizeof(HashBits) == sizeof(usize));

		struct Entry : NonMovable {
			union {
				pair_type key_value;
			};

			Entry() {
			}

			~Entry() {
			}

			void set(pair_type&& kv) {
				::new(&key_value) pair_type{std::move(kv)};
			}

			void clear() {
				key_value.~pair_type();
			}

			const key_type& key() const {
				return key_value.first;
			}
		};

	public:
		static constexpr double max_load_factor = detail::default_dense_map_max_load_factor;
		static constexpr usize min_capacity = 16;

		auto key_values() const {
			return core::Range(
				make_iterator(_bits.begin()),
				EndIterator{}
			);
		}

		auto key_values() {
			return core::Range(
				make_iterator(_bits.begin()),
				EndIterator{}
			);
		}

		auto values() const {
			return core::Range(
				TransformIterator(make_iterator(_bits.begin()), [](const pair_type& entry) -> const mapped_type& {
					return entry.key_value.second;
				}),
				EndIterator{}
			);
		}

		auto values() {
			return core::Range(
				TransformIterator(make_iterator(_bits.begin()), [](pair_type& entry) -> mapped_type& {
					return entry.key_value.second;
				}),
				EndIterator{}
			);
		}

		auto keys() const {
			return core::Range(
				TransformIterator(make_iterator(_bits.begin()), [](const pair_type& entry) -> const key_type& {
					return entry.key();
				}),
				EndIterator{}
			);
		}

	private:
		auto bits_iterator(const HashBits* bits) const {
			return FilterIterator(bits, _bits.end(), [](const HashBits& b) { return b.is_full(); });
		}

		auto bits_iterator(HashBits* bits) {
			return FilterIterator(bits, _bits.end(), [](const HashBits& b) { return b.is_full(); });
		}

		auto make_iterator(const HashBits* bits) const {
			const HashBits* bits_beg = _bits.data();
			const Entry* entries = _entries.get();
			return TransformIterator(bits_iterator(bits), [bits_beg, entries](const HashBits& b) -> const value_type& {
				const usize index = &b - bits_beg;
				return detail::map_entry_to_value_type(entries[index].key_value);
			});
		}

		auto make_iterator(HashBits* bits) {
			const HashBits* bits_beg = _bits.data();
			Entry* entries = _entries.get();
			return TransformIterator(bits_iterator(bits), [bits_beg, entries](const HashBits& b) -> value_type& {
				const usize index = &b - bits_beg;
				return detail::map_entry_to_value_type(entries[index].key_value);
			});
		}


		bool should_expand() const {
			return _bits.size() * max_load_factor <= _size;
		}

		usize hash(const key_type& key) const {
			return Hasher::operator()(key);
		}

		Bucket find_bucket_for_insert(const key_type& key) {
			const usize h = hash(key);
			return find_bucket_for_insert(key, h);
		}

		Bucket find_bucket_for_insert(const key_type& key, usize h) {
			const usize entry_count = _bits.size();
			const usize hash_mask = entry_count - 1;
			usize probes = 0;

			{
				usize best_index = usize(-1);
				for(; probes <= _max_probe_len; ++probes) {
					const usize index = (h + detail::probing_offset<>(probes)) & hash_mask;
					const HashBits& bits = _bits[index];
					if(!bits.is_full()) {
						best_index = index;
						if(bits.is_empty_strict()) {
							return {best_index, h};
						}
					} else if(bits.is_hash(h) && _entries[index].key() == key) {
						return {index, h};
					}
				}

				if(best_index != usize(-1)) {
					return {best_index, h};
				}
			}

			for(; probes != entry_count; ++probes) {
				const usize index = (h + detail::probing_offset<>(probes)) & hash_mask;
				if(!_bits[index].is_full()) {
					_max_probe_len = probes;
					return {index, h};
				}
			}

			y_fatal("Internal error: unable to find empty bucket");
		}

		const HashBits* find_bits(const key_type& key) const {
			const usize h = hash(key);
			const usize entry_count = _bits.size();
			const usize hash_mask = entry_count - 1;
			for(usize i = 0; i != entry_count; ++i) {
				const usize index = (h + detail::probing_offset<>(i)) & hash_mask;
				const HashBits& bits = _bits[index];
				if(bits.is_hash(h) && _entries[index].key() == key) {
					return &bits;
				}
				if(bits.is_empty_strict()) {
					return nullptr;
				}
			}
			return nullptr;
		}

		void expand(usize new_entry_count) {
			const usize pow_2 = detail::ceil_next_power_of_2(new_entry_count);
			const usize new_size = pow_2 < min_capacity ? min_capacity : pow_2;

			if(new_size <= _bits.size()) {
				return;
			}

			auto old_bits = std::exchange(_bits, FixedArray<HashBits>(new_size));
			auto old_entries = std::exchange(_entries, std::make_unique<Entry[]>(new_size));
			_max_probe_len = 0;

			if(_size) {
				const usize current_entry_count = old_bits.size();
				for(usize i = 0; i != current_entry_count; ++i) {
					if(old_bits[i].is_full()) {
						const Bucket bucket = find_bucket_for_insert(old_entries[i].key(), old_bits[i].hash());
						const usize new_index = bucket.index;

						y_debug_assert(!_bits[new_index].is_full());

						_bits[new_index].set_hash(bucket.hash);
						_entries[new_index].set(std::move(old_entries[i].key_value));
					}
				}
			}
		}

		void expand() {
			expand(_bits.size() == 0 ? min_capacity : 2 * _bits.size());
		}

		void audit() {
#ifdef Y_DENSEMAP_AUDIT
			usize entry_count = 0;
			for(usize i = 0; i != _bits.size(); ++i) {
				if(!_bits[i].is_full()) {
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

		FixedArray<HashBits> _bits;
		std::unique_ptr<Entry[]> _entries;
		usize _size = 0;
		usize _max_probe_len = 0;

	public:
		using iterator			= typename decltype(std::declval<      ExternalBitsDenseMap<Key, Value, Hasher>>().key_values())::iterator;
		using const_iterator	= typename decltype(std::declval<const ExternalBitsDenseMap<Key, Value, Hasher>>().key_values())::iterator;

		static_assert(std::is_copy_assignable_v<const_iterator>);
		static_assert(std::is_copy_constructible_v<const_iterator>);

		ExternalBitsDenseMap() = default;
		ExternalBitsDenseMap(ExternalBitsDenseMap&& other) {
			swap(other);
		}

		ExternalBitsDenseMap& operator=(ExternalBitsDenseMap&& other) {
			swap(other);
			return *this;
		}

		void swap(ExternalBitsDenseMap& other) {
			if(&other != this) {
				std::swap(_bits, other._bits);
				std::swap(_entries, other._entries);
				std::swap(_size, other._size);
				std::swap(_max_probe_len, other._max_probe_len);
			}
		}

		~ExternalBitsDenseMap() {
			const usize len = _bits.size();
			for(usize i = 0; i != len && _size; ++i) {
				if(_bits[i].is_full()) {
					_entries[i].clear();
					--_size;
				}
			}
		}

		iterator begin() {
			return key_values().begin();
		}

		const_iterator begin() const {
			return key_values().begin();
		}

		auto end() {
			return EndIterator{};
		}

		auto end() const {
			return EndIterator{};
		}


		usize size() const {
			return _size;
		}

		double load_factor() const {
			return double(_size) / double(_entries.size());
		}

		bool contains(const key_type& key) const {
			return find_bits(key);
		}

		iterator find(const key_type& key) {
			HashBits* bits = const_cast<HashBits*>(find_bits(key));
			return make_iterator(bits ? bits : _bits.end());
		}

		const_iterator find(const key_type& key) const {
			const HashBits* bits = find_bits(key);
			return make_iterator(bits ? bits : _bits.end());
		}

		void rehash() {
			expand(_bits.size());
		}

		void set_min_capacity(usize cap) {
			const usize capacity = usize(cap / (max_load_factor * 0.8));
			if(_bits.size() < capacity) {
				expand(capacity);
			}
		}

		void reserve(usize cap) {
			set_min_capacity(cap);
		}

		template<typename... Args>
		std::pair<iterator, bool> emplace(const key_type& key, Args&&... args) {
			return insert(pair_type{key, mapped_type{y_fwd(args)...}});
		}

		std::pair<iterator, bool> insert(pair_type p) {
			y_defer(audit());

			if(should_expand()) {
				expand();
			}

			y_debug_assert(!should_expand());

			const Bucket bucket = find_bucket_for_insert(p.first);
			const usize index = bucket.index;
			const bool exists = _bits[index].is_full();

			if(!exists) {
				_entries[index].set(std::move(p));
				_bits[index].set_hash(bucket.hash);
				++_size;
			}

			return {make_iterator(&_bits[index]), !exists};
		}

		void erase(const iterator& it) {
			y_defer(audit());

			HashBits* bits_ptr = it.inner().inner();

			y_debug_assert(bits_ptr >= _bits.data());
			y_debug_assert(bits_ptr < _bits.data() + _bits.size());

			const usize index = bits_ptr - _bits .data();
			_entries[index].clear();
			bits_ptr->make_empty();

			--_size;
		}
};
}

namespace vector {
template<typename Key, typename Value, typename Hasher = std::hash<Key>>
class ExternalDenseMap : Hasher {
	public:
		using key_type = remove_cvref_t<Key>;
		using mapped_type = remove_cvref_t<Value>;
		using value_type = std::pair<const key_type, mapped_type>;

	private:
		using pair_type = std::pair<key_type, mapped_type>;

		struct Bucket {
			usize index;
			usize hash;
		};

		struct HashBits {
			static constexpr usize empty_index = usize(-1);
			static constexpr usize tombstone_index = usize(-2);

			usize bits = 0;
			usize value_index = empty_index;

			void set_hash(usize hash, usize index) {
				y_debug_assert(!is_full());
				bits = hash;
				value_index = index;
			}

			void make_empty() {
				y_debug_assert(is_full());
				value_index = tombstone_index;
			}

			bool is_full() const {
				return value_index < tombstone_index;
			}

			bool is_hash(usize hash) const {
				return is_full() && bits == hash;
			}

			bool is_empty_strict() const {
				return value_index == empty_index;
			}

			bool is_tombstone() const {
				return value_index = tombstone_index;
			}

			usize hash() const {
				y_debug_assert(is_full());
				return bits;
			}
		};

	public:
		static constexpr double max_load_factor = detail::default_dense_map_max_load_factor;
		static constexpr usize min_capacity = 16;

		core::Span<value_type> key_values() const {
			return {detail::map_entry_to_value_type_ptr(_entries.data()), _entries.size()};
		}

		core::MutableSpan<value_type> key_values() {
			return {detail::map_entry_to_value_type_ptr(_entries.data()), _entries.size()};
		}

		auto values() const {
			return core::Range(
				TransformIterator(_entries.begin(), [](const pair_type& entry) -> const mapped_type& {
					return entry.second;
				}),
				_entries.end()
			);
		}

		auto values() {
			return core::Range(
				TransformIterator(_entries.begin(), [](pair_type& entry) -> mapped_type& {
					return entry.second;
				}),
				_entries.end()
			);
		}

		auto keys() const {
			return core::Range(
				TransformIterator(_entries.begin(), [](const pair_type& entry) -> const key_type& {
					return entry.first;
				}),
				_entries.end()
			);
		}

	private:
		bool should_expand() const {
			return _bits.size() * max_load_factor <= _entries.size();
		}

		usize hash(const key_type& key) const {
			return Hasher::operator()(key);
		}

		Bucket find_bucket_for_insert(const key_type& key) {
			const usize h = hash(key);
			return find_bucket_for_insert(key, h);
		}

		Bucket find_bucket_for_insert(const key_type& key, usize h) {
			const usize entry_count = _bits.size();
			const usize hash_mask = entry_count - 1;
			usize probes = 0;

			{
				usize best_index = usize(-1);
				for(; probes <= _max_probe_len; ++probes) {
					const usize index = (h + detail::probing_offset<>(probes)) & hash_mask;
					const HashBits& bits = _bits[index];
					if(!bits.is_full()) {
						best_index = index;
						if(bits.is_empty_strict()) {
							return {best_index, h};
						}
					} else if(bits.is_hash(h) && _entries[bits.value_index].first == key) {
						return {index, h};
					}
				}

				if(best_index != usize(-1)) {
					return {best_index, h};
				}
			}

			for(; probes != entry_count; ++probes) {
				const usize index = (h + detail::probing_offset<>(probes)) & hash_mask;
				if(!_bits[index].is_full()) {
					_max_probe_len = probes;
					return {index, h};
				}
			}

			y_fatal("Internal error: unable to find empty bucket");
		}

		const HashBits* find_bits(const key_type& key) const {
			const usize h = hash(key);
			const usize entry_count = _bits.size();
			const usize hash_mask = entry_count - 1;
			for(usize i = 0; i <= _max_probe_len; ++i) {
				const usize index = (h + detail::probing_offset<>(i)) & hash_mask;
				const HashBits& bits = _bits[index];
				if(bits.is_hash(h) && _entries[bits.value_index].first == key) {
					return &bits;
				}
				if(bits.is_empty_strict()) {
					return nullptr;
				}
			}
			return nullptr;
		}

		void expand(usize new_entry_count) {
			const usize pow_2 = detail::ceil_next_power_of_2(new_entry_count);
			const usize new_size = pow_2 < min_capacity ? min_capacity : pow_2;

			if(new_size <= _bits.size()) {
				return;
			}

			auto old_bits = std::exchange(_bits, FixedArray<HashBits>(new_size));
			_max_probe_len = 0;

			const usize current_entry_count = old_bits.size();
			for(usize i = 0; i != current_entry_count; ++i) {
				const HashBits& bits = old_bits[i];
				if(bits.is_full()) {
					const Bucket bucket = find_bucket_for_insert(_entries[bits.value_index].first, bits.hash());
					const usize new_index = bucket.index;

					y_debug_assert(!_bits[new_index].is_full());

					_bits[new_index].set_hash(bucket.hash, bits.value_index);
				}
			}
		}

		void expand() {
			expand(_bits.size() == 0 ? min_capacity : 2 * _bits.size());
		}

		void audit() {
#ifdef Y_DENSEMAP_AUDIT
			usize entry_count = 0;
			for(usize i = 0; i != _bits.size(); ++i) {
				if(!_bits[i].is_full()) {
					continue;
				}
				++entry_count;
				const key_type& k = _entries[_bits[i].value_index].first;
				const Bucket b = find_bucket_for_insert(k);
				y_debug_assert(b.index == i);
			}
			y_debug_assert(entry_count == _entries.size());
#endif
		}

		FixedArray<HashBits> _bits;
		core::Vector<pair_type> _entries;
		usize _max_probe_len = 0;

	public:
		using iterator			= typename decltype(std::declval<      ExternalDenseMap<Key, Value, Hasher>>().key_values())::iterator;
		using const_iterator	= typename decltype(std::declval<const ExternalDenseMap<Key, Value, Hasher>>().key_values())::iterator;

		static_assert(std::is_copy_assignable_v<const_iterator>);
		static_assert(std::is_copy_constructible_v<const_iterator>);

		ExternalDenseMap() = default;
		ExternalDenseMap(ExternalDenseMap&& other) {
			swap(other);
		}

		ExternalDenseMap& operator=(ExternalDenseMap&& other) {
			swap(other);
			return *this;
		}

		void swap(ExternalDenseMap& other) {
			if(&other != this) {
				std::swap(_bits, other._bits);
				std::swap(_entries, other._entries);
				std::swap(_max_probe_len, other._max_probe_len);
			}
		}

		iterator begin() {
			return key_values().begin();
		}

		const_iterator begin() const {
			return key_values().begin();
		}

		auto end() {
			return key_values().end();
		}

		auto end() const {
			return key_values().end();
		}


		usize size() const {
			return _entries.size();
		}

		double load_factor() const {
			return double(_entries.size()) / double(_entries.size());
		}

		bool contains(const key_type& key) const {
			return find_bits(key);
		}

		iterator find(const key_type& key) {
			const HashBits* bits = find_bits(key);
			auto kv = key_values();
			return bits ? &kv[bits->value_index] : kv.end();
		}

		const_iterator find(const key_type& key) const {
			const HashBits* bits = find_bits(key);
			const auto kv = key_values();
			return bits ? &kv[bits->value_index] : kv.end();
		}

		void rehash() {
			expand(_bits.size());
		}

		void set_min_capacity(usize cap) {
			_entries.set_min_capacity(cap);
			const usize capacity = usize(cap / (max_load_factor * 0.8));
			if(_bits.size() < capacity) {
				expand(capacity);
			}
		}

		void reserve(usize cap) {
			set_min_capacity(cap);
		}

		template<typename... Args>
		std::pair<iterator, bool> emplace(const key_type& key, Args&&... args) {
			return insert(pair_type{key, mapped_type{y_fwd(args)...}});
		}

		std::pair<iterator, bool> insert(pair_type p) {
			y_defer(audit());

			if(should_expand()) {
				expand();
			}

			y_debug_assert(!should_expand());

			const Bucket bucket = find_bucket_for_insert(p.first);
			const usize index = bucket.index;
			const bool exists = _bits[index].is_full();

			if(!exists) {
				_bits[index].set_hash(bucket.hash, _entries.size());
				_entries.emplace_back(std::move(p));
			}

			return {&key_values()[_bits[index].value_index], !exists};
		}

		void erase(const iterator& it) {
			y_defer(audit());

			const pair_type* entry_ptr = reinterpret_cast<const pair_type*>(it);
			const usize index = entry_ptr - _entries.data();

			HashBits* bits = const_cast<HashBits*>(find_bits(entry_ptr->first));

			y_debug_assert(bits && bits->is_full());
			y_debug_assert(&_entries[bits->value_index] == entry_ptr);

			if(entry_ptr != &_entries.last()) {
				HashBits* last_bits = const_cast<HashBits*>(find_bits(_entries.last().first));
				last_bits->value_index = index;
				std::swap(_entries[index], _entries.last());
			}

			_entries.pop();
			bits->make_empty();
		}
};
}

using namespace compact;
using namespace bits;
using namespace vector;

}
}

#endif // Y_CORE_DENSEMAP_H
