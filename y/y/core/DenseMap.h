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

namespace external {
template<typename Key, typename Value, typename Hasher = std::hash<Key>>
class ExternalDenseMap : Hasher {

	using key_type = remove_cvref_t<Key>;
	using value_type = remove_cvref_t<Value>;

	using index_type = u32;
	static constexpr index_type empty_index = index_type(-1);
	static constexpr index_type tombstone_index = index_type(-2);

	struct ValueBox {
		value_type value;
		index_type key_index;
	};

	struct KeyBox : NonCopyable {
		union {
			key_type key;
		};
		index_type value_index = empty_index;

		KeyBox() = default;

		~KeyBox() {
			make_empty();
		}

		KeyBox(KeyBox&& other) {
			operator=(std::move(other));
		}

		KeyBox& operator=(KeyBox&& other) {
			make_empty();
			if(other.has_key()) {
				value_index = other.value_index;
				key = std::move(other.key);
			}
			return *this;
		}

		bool has_key() const {
			return value_index < tombstone_index;
		}

		bool is_empty_strict() const {
			return value_index == empty_index;
		}

		bool is_tombstone() const {
			return value_index == tombstone_index;
		}

		const Key& get() const {
			y_debug_assert(has_key());
			return key;
		}

		void emplace(const Key& k, index_type i) {
			y_debug_assert(!has_key());
			value_index = i;
			new(&key) key_type{k};
		}

		void make_empty() {
			if(has_key()) {
				value_index = tombstone_index;
				key.~key_type();
			}
		}

		bool operator==(const Key& k) const {
			return has_key() && key == k;
		}
	};

	public:
		static constexpr double max_load_factor = 0.7;
		static constexpr usize min_capacity = 16;

		auto key_values() const {
			return core::Range(make_iterator(_values.begin()), _values.end());
		}

		auto key_values() {
			return core::Range(make_iterator(_values.begin()), _values.end());
		}

		auto values() const {
			auto tr = [this](const ValueBox& box) -> const Value& { return box.value; };
			return core::Range(
				TransformIterator(_values.begin(), tr),
				_values.end()
			);
		}

		auto values() {
			auto tr = [this](ValueBox& box) -> Value& { return box.value; };
			return core::Range(
				TransformIterator(_values.begin(), tr),
				_values.end()
			);
		}

		auto keys() const {
			return core::Range(
				TransformIterator(key_begin_iterator(), [](const KeyBox& box) -> const Key& {
					return box.get();
				}),
				EndIterator{}
			);
		}

	private:
		auto make_iterator(const ValueBox* inner) const {
			auto tr = [this](const ValueBox& box) -> std::pair<const Key&, const Value&> {
				return {_keys[box.key_index].get(), box.value};
			};
			return TransformIterator(inner, tr);
		}

		auto make_iterator(ValueBox* inner) {
			auto tr = [this](ValueBox& box) -> std::pair<const Key&, Value&> {
				return {_keys[box.key_index].get(), box.value};
			};
			return TransformIterator(inner, tr);
		}

		auto key_begin_iterator() const {
			return FilterIterator(_keys.begin(), _keys.end(), [](const KeyBox& box) { return box.has_key(); });
		}

		bool should_expand() const {
			return _keys.size() * max_load_factor <=  _values.size();
		}

		usize hash(const Key& key) const {
			return Hasher::operator()(key);
		}

		// http://research.cs.vt.edu/AVresearch/hashing/quadratic.php
		static constexpr usize probing_offset(usize i) {
			return (i * i + i) / 2;
		}

		index_type find_bucket_index_for_insert(const Key& key) const {
			const usize h = hash(key);
			const usize key_count = _keys.size();
			index_type best_index = 0;
			for(usize i = 0; i != key_count; ++i) {
				const index_type key_index = (h + probing_offset(i)) % key_count;
				const KeyBox& box = _keys[key_index];
				if(!box.has_key()) {
					best_index = key_index;
					if(box.is_empty_strict()) {
						return best_index;
					}
				} else if(box == key) {
					return key_index;
				}
			}
			return best_index;
		}

		const KeyBox* find_key(const Key& key) const {
			const usize h = hash(key);
			const usize key_count = _keys.size();
			for(usize i = 0; i != key_count; ++i) {
				const index_type key_index = (h + probing_offset(i)) % key_count;
				const KeyBox& box = _keys[key_index];
				if(box == key) {
					return &box;
				}
				if(box.is_empty_strict()) {
					return nullptr;
				}
			}
			return nullptr;
		}

		static constexpr usize ceil_next_power_of_2(usize k) {
			return 1 << log2ui(k + 1);
		}

		void expand(usize new_key_count) {
			const usize pow_2 = ceil_next_power_of_2(new_key_count);
			const usize new_size = pow_2 < min_capacity ? min_capacity : pow_2;

			auto old_keys = std::exchange(_keys, FixedArray<KeyBox>(new_size));
			for(KeyBox& k : old_keys) {
				if(k.has_key()) {
					const index_type new_index = find_bucket_index_for_insert(k.key);
					y_debug_assert(!_keys[new_index].has_key());
					_keys[new_index].emplace(std::move(k.key), k.value_index);
					_values[k.value_index].key_index = new_index;
				}
			}
		}

		void expand() {
			expand(_keys.size() == 0 ? min_capacity : 2 * _keys.size());
		}

		void audit() const {
#ifdef Y_DENSEMAP_AUDIT
			y_debug_assert(_keys.size() >= _values.size() + 1);
			for(index_type i = 0; i != _values.size(); ++i) {
				const KeyBox& k = _keys[_values[i].key_index];
				y_debug_assert(k.has_key());
				y_debug_assert(k.value_index == i);
			}
			usize key_count = 0;
			for(index_type i = 0; i != _keys.size(); ++i) {
				const KeyBox& k = _keys[i];
				if(!k.has_key()) {
					continue;
				}
				++key_count;
				const ValueBox& v = _values[k.value_index];
				y_debug_assert(v.key_index == i);
				y_debug_assert(find_key(k.key) == &k);
			}
			y_debug_assert(_values.size() == key_count);
#endif
		}

		FixedArray<KeyBox> _keys;
		Vector<ValueBox> _values;

	public:
		using iterator			= typename decltype(std::declval<      ExternalDenseMap<Key, Value, Hasher>>().key_values())::iterator;
		using const_iterator	= typename decltype(std::declval<const ExternalDenseMap<Key, Value, Hasher>>().key_values())::iterator;

		static_assert(std::is_copy_assignable_v<const_iterator>);
		static_assert(std::is_copy_constructible_v<const_iterator>);

		iterator begin() {
			return make_iterator(_values.begin());
		}

		const_iterator begin() const {
			return make_iterator(_values.begin());
		}

		auto end() {
			return make_iterator(_values.end());
		}

		auto end() const {
			return make_iterator(_values.end());
		}


		usize size() const {
			return _values.size();
		}

		double load_factor() const {
			return double(_values.size()) / double(_keys.size());
		}

		bool contains(const Key& key) const {
			return find_key(key);
		}

		iterator find(const Key& key) {
			const KeyBox* box = find_key(key);
			return make_iterator(box ? &_values[box->value_index] : _values.end());
		}

		const_iterator find(const Key& key) const {
			const KeyBox* box = find_key(key);
			return make_iterator(box ? &_values[box->value_index] : _values.end());
		}

		void rehash() {
			expand(_keys.size());
		}

		void set_min_capacity(usize cap) {
			if(cap > _values.capacity()) {
				_values.set_min_capacity(cap);
			}
			const usize key_capacity = usize(cap / (max_load_factor * 0.8));
			if(_keys.size() < key_capacity) {
				expand(key_capacity);
			}
		}

		void reserve(usize cap) {
			set_min_capacity(cap);
		}

		template<typename... Args>
		std::pair<iterator, bool> emplace(const Key& key, Args&&... args) {
			y_defer(audit());

			if(should_expand()) {
				expand();
			}

			y_debug_assert(!should_expand());

			const index_type value_index = _values.size();
			const index_type key_index = find_bucket_index_for_insert(key);
			if(_keys[key_index].has_key()) {
				return {make_iterator(&_values[_keys[key_index].value_index]), false};
			}

			_values.emplace_back(ValueBox{value_type{y_fwd(args)...}, key_index});
			_keys[key_index].emplace(key, value_index);

			return {make_iterator(&_values.last()), true};
		}

		std::pair<iterator, bool> insert(std::pair<const Key, Value> p) {
			return emplace(p.first, std::move(p.second));
		}

		void erase(const iterator& it) {
			y_defer(audit());

			ValueBox* value_ptr = it.inner();

			y_debug_assert(value_ptr >= _values.data());
			y_debug_assert(value_ptr < _values.data() + _values.size());

			const index_type key_index = value_ptr->key_index;
			KeyBox* key_ptr = &_keys[key_index];
			const index_type value_index = key_ptr->value_index;

			if(value_ptr != &_values.last()) {
				std::swap(*value_ptr, _values.last());
				_keys[_values[value_index].key_index].value_index = value_index;
			}

			_values.pop();
			key_ptr->make_empty();
		}
};
}

namespace stable {
// Improvements ideas:
//		pool value allocations
//		inline value if small (will break stability)
template<typename Key, typename Value, typename Hasher = std::hash<Key>>
class StableDenseMap : Hasher {

	using key_type = remove_cvref_t<Key>;
	using value_type = remove_cvref_t<Value>;

	struct Entry : NonCopyable {
		union {
			key_type key;
		};
		bool is_tombstone = false;
		std::unique_ptr<value_type> value;

		Entry() = default;

		~Entry() {
			make_empty();
		}

		Entry(Entry&& other) {
			operator=(std::move(other));
		}

		Entry& operator=(Entry&& other) {
			make_empty();
			if(other.has_key()) {
				key = std::move(other.key);
				value = std::move(other.value);
			}
			return *this;
		}

		bool has_key() const {
			return value != nullptr;
		}

		bool is_empty_strict() const {
			return !has_key() && !is_tombstone;
		}

		const Key& get() const {
			y_debug_assert(has_key());
			return key;
		}

		template<typename... Args>
		void emplace(const Key& k, Args&&... args) {
			set(k, std::make_unique<value_type>(y_fwd(args)...));
		}

		void set(const Key& k, std::unique_ptr<value_type> ptr) {
			y_debug_assert(!has_key());
			new(&key) key_type{k};
			value = std::move(ptr);
		}

		void make_empty() {
			if(has_key()) {
				is_tombstone = true;
				key.~key_type();
				value = nullptr;
			}
		}

		bool operator==(const Key& k) const {
			return has_key() && key == k;
		}
	};

	public:
		static constexpr double max_load_factor = 0.7;
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
				TransformIterator(entry_iterator(_entries.begin()), [](const Entry& entry) -> const Value& {
					return *entry.value;
				}),
				EndIterator{}
			);
		}

		auto values() {
			return core::Range(
				TransformIterator(entry_iterator(_entries.begin()), [](Entry& entry) -> Value& {
					return *entry.value;
				}),
				EndIterator{}
			);
		}

		auto keys() const {
			return core::Range(
				TransformIterator(entry_iterator(_entries.begin()), [](const Entry& entry) -> const Key& {
					return entry.get();
				}),
				EndIterator{}
			);
		}

	private:
		auto entry_iterator(const Entry* entry) const {
			return FilterIterator(entry, _entries.end(), [](const Entry& entry) { return entry.has_key(); });
		}

		auto entry_iterator(Entry* entry) {
			return FilterIterator(entry, _entries.end(), [](Entry& entry) { return entry.has_key(); });
		}

		auto make_iterator(const Entry* entry) const {
			return TransformIterator(entry_iterator(entry), [](const Entry& entry) -> std::pair<const Key&, const Value&> {
				return {entry.get(), *entry.value};
			});
		}

		auto make_iterator(Entry* entry) {
			return TransformIterator(entry_iterator(entry), [](Entry& entry) -> std::pair<const Key&, Value&> {
				return {entry.get(), *entry.value};
			});
		}


		bool should_expand() const {
			return _entries.size() * max_load_factor <=  _size;
		}

		usize hash(const Key& key) const {
			return Hasher::operator()(key);
		}

		// http://research.cs.vt.edu/AVresearch/hashing/quadratic.php
		static constexpr usize probing_offset(usize i) {
			return (i * i + i) / 2;
		}

		usize find_bucket_index_for_insert(const Key& key) const {
			const usize h = hash(key);
			const usize entry_count = _entries.size();
			usize best_index = 0;
			for(usize i = 0; i != entry_count; ++i) {
				const usize index = (h + probing_offset(i)) % entry_count;
				const Entry& entry = _entries[index];
				if(!entry.has_key()) {
					best_index = index;
					if(entry.is_empty_strict()) {
						return best_index;
					}
				} else if(entry == key) {
					return index;
				}
			}
			return best_index;
		}

		const Entry* find_key(const Key& key) const {
			const usize h = hash(key);
			const usize entry_count = _entries.size();
			for(usize i = 0; i != entry_count; ++i) {
				const usize index = (h + probing_offset(i)) % entry_count;
				const Entry& entry = _entries[index];
				if(entry == key) {
					return &entry;
				}
				if(entry.is_empty_strict()) {
					return nullptr;
				}
			}
			return nullptr;
		}

		static constexpr usize ceil_next_power_of_2(usize k) {
			return 1 << log2ui(k + 1);
		}

		void expand(usize new_entry_count) {
			const usize pow_2 = ceil_next_power_of_2(new_entry_count);
			const usize new_size = pow_2 < min_capacity ? min_capacity : pow_2;

			auto old_entries = std::exchange(_entries, FixedArray<Entry>(new_size));
			for(Entry& k : old_entries) {
				if(k.has_key()) {
					const usize new_index = find_bucket_index_for_insert(k.key);
					y_debug_assert(!_entries[new_index].has_key());
					_entries[new_index].set(std::move(k.key), std::move(k.value));
				}
			}
		}

		void expand() {
			expand(_entries.size() == 0 ? min_capacity : 2 * _entries.size());
		}

		void audit() const {
#ifdef Y_DENSEMAP_AUDIT
			y_debug_assert(_entries.size() >= _size + 1);
			usize entry_count = 0;
			for(usize i = 0; i != _entries.size(); ++i) {
				const Entry& k = _entries[i];
				if(!k.has_key()) {
					continue;
				}
				++entry_count;
				y_debug_assert(find_key(k.key) == &k);
			}
			y_debug_assert(_size == entry_count);
#endif
		}

		FixedArray<Entry> _entries;
		usize _size = 0;

	public:
		using iterator			= typename decltype(std::declval<      StableDenseMap<Key, Value, Hasher>>().key_values())::iterator;
		using const_iterator	= typename decltype(std::declval<const StableDenseMap<Key, Value, Hasher>>().key_values())::iterator;

		static_assert(std::is_copy_assignable_v<const_iterator>);
		static_assert(std::is_copy_constructible_v<const_iterator>);

		StableDenseMap() = default;
		StableDenseMap(StableDenseMap&& other) {
			operator=(std::move(other));
		}

		StableDenseMap& operator=(StableDenseMap&& other) {
			std::swap(_entries, other._entries);
			std::swap(_size, other._size);
			return *this;
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

		bool contains(const Key& key) const {
			return find_key(key);
		}

		iterator find(const Key& key) {
			Entry* entry = const_cast<Entry*>(find_key(key));
			return make_iterator(entry ? entry : _entries.end());
		}

		const_iterator find(const Key& key) const {
			const Entry* entry = find_key(key);
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
		std::pair<iterator, bool> emplace(const Key& key, Args&&... args) {
			y_defer(audit());

			if(should_expand()) {
				expand();
			}

			y_debug_assert(!should_expand());

			const usize index = find_bucket_index_for_insert(key);
			const bool exists = _entries[index].has_key();

			if(!exists) {
				_entries[index].emplace(key, y_fwd(args)...);
				++_size;
			}

			return {make_iterator(&_entries[index]), !exists};
		}

		std::pair<iterator, bool> insert(std::pair<const Key, Value> p) {
			return emplace(p.first, std::move(p.second));
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

namespace compact {
template<typename Key, typename Value, typename Hasher = std::hash<Key>>
class DenseMap : Hasher {

	using key_type = remove_cvref_t<Key>;
	using value_type = remove_cvref_t<Value>;

	enum class EntryState : u8 {
		Empty = 0,
		Tombstone = 1,
		Full = 2
	};

	struct Entry : NonCopyable {
		union {
			key_type key;
		};

		EntryState state = EntryState::Empty;

		union {
			value_type value;
		};

		Entry() = default;

		~Entry() {
			make_empty();
		}

		Entry(Entry&& other) {
			operator=(std::move(other));
		}

		Entry& operator=(Entry&& other) {
			make_empty();
			if(other.has_key()) {
				state = EntryState::Full;
				key = std::move(other.key);
				value = std::move(other.value);
			}
			return *this;
		}

		bool has_key() const {
			return state == EntryState::Full;
		}

		bool is_empty_strict() const {
			return state == EntryState::Empty;
		}

		bool is_tombstone() const {
			return state == EntryState::Tombstone;
		}

		template<typename... Args>
		void emplace(const Key& k, Args&&... args) {
			y_debug_assert(!has_key());
			state = EntryState::Full;
			new(&key) key_type{k};
			new(&value) value_type{y_fwd(args)...};
		}

		void make_empty() {
			if(has_key()) {
				state = EntryState::Tombstone;
				key.~key_type();
				value.~value_type();
			}
		}

		bool operator==(const Key& k) const {
			return has_key() && key == k;
		}
	};

	public:
		static constexpr double max_load_factor = 0.7;
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
				TransformIterator(entry_iterator(_entries.begin()), [](const Entry& entry) -> const Value& {
					return entry.value;
				}),
				EndIterator{}
			);
		}

		auto values() {
			return core::Range(
				TransformIterator(entry_iterator(_entries.begin()), [](Entry& entry) -> Value& {
					return entry.value;
				}),
				EndIterator{}
			);
		}

		auto keys() const {
			return core::Range(
				TransformIterator(entry_iterator(_entries.begin()), [](const Entry& entry) -> const Key& {
					return entry.key;
				}),
				EndIterator{}
			);
		}

	private:
		auto entry_iterator(const Entry* entry) const {
			return FilterIterator(entry, _entries.end(), [](const Entry& entry) { return entry.has_key(); });
		}

		auto entry_iterator(Entry* entry) {
			return FilterIterator(entry, _entries.end(), [](Entry& entry) { return entry.has_key(); });
		}

		auto make_iterator(const Entry* entry) const {
			return TransformIterator(entry_iterator(entry), [](const Entry& entry) -> std::pair<const Key&, const Value&> {
				return {entry.key, entry.value};
			});
		}

		auto make_iterator(Entry* entry) {
			return TransformIterator(entry_iterator(entry), [](Entry& entry) -> std::pair<const Key&, Value&> {
				return {entry.key, entry.value};
			});
		}


		bool should_expand() const {
			return _entries.size() * max_load_factor <=  _size;
		}

		usize hash(const Key& key) const {
			return Hasher::operator()(key);
		}

		// http://research.cs.vt.edu/AVresearch/hashing/quadratic.php
		static constexpr usize probing_offset(usize i) {
			return (i * i + i) / 2;
		}

		usize find_bucket_index_for_insert(const Key& key) const {
			const usize h = hash(key);
			const usize entry_count = _entries.size();
			usize best_index = 0;
			for(usize i = 0; i != entry_count; ++i) {
				const usize index = (h + probing_offset(i)) % entry_count;
				const Entry& entry = _entries[index];
				if(!entry.has_key()) {
					best_index = index;
					if(entry.is_empty_strict()) {
						return best_index;
					}
				} else if(entry == key) {
					return index;
				}
			}
			return best_index;
		}

		const Entry* find_key(const Key& key) const {
			const usize h = hash(key);
			const usize entry_count = _entries.size();
			for(usize i = 0; i != entry_count; ++i) {
				const usize index = (h + probing_offset(i)) % entry_count;
				const Entry& entry = _entries[index];
				if(entry == key) {
					return &entry;
				}
				if(entry.is_empty_strict()) {
					return nullptr;
				}
			}
			return nullptr;
		}

		static constexpr usize ceil_next_power_of_2(usize k) {
			return 1 << log2ui(k + 1);
		}

		void expand(usize new_entry_count) {
			const usize pow_2 = ceil_next_power_of_2(new_entry_count);
			const usize new_size = pow_2 < min_capacity ? min_capacity : pow_2;

			auto old_entries = std::exchange(_entries, FixedArray<Entry>(new_size));
			for(Entry& k : old_entries) {
				if(k.has_key()) {
					const usize new_index = find_bucket_index_for_insert(k.key);
					y_debug_assert(!_entries[new_index].has_key());
					_entries[new_index].emplace(std::move(k.key), std::move(k.value));
				}
			}
		}

		void expand() {
			expand(_entries.size() == 0 ? min_capacity : 2 * _entries.size());
		}

		void audit() const {
#ifdef Y_DENSEMAP_AUDIT
			y_debug_assert(_entries.size() >= _size + 1);
			usize entry_count = 0;
			for(usize i = 0; i != _entries.size(); ++i) {
				const Entry& k = _entries[i];
				if(!k.has_key()) {
					continue;
				}
				++entry_count;
				y_debug_assert(find_key(k.key) == &k);
			}
			y_debug_assert(_size == entry_count);
#endif
		}

		FixedArray<Entry> _entries;
		usize _size = 0;

	public:
		using iterator			= typename decltype(std::declval<      DenseMap<Key, Value, Hasher>>().key_values())::iterator;
		using const_iterator	= typename decltype(std::declval<const DenseMap<Key, Value, Hasher>>().key_values())::iterator;

		static_assert(std::is_copy_assignable_v<const_iterator>);
		static_assert(std::is_copy_constructible_v<const_iterator>);

		DenseMap() = default;
		DenseMap(DenseMap&& other) {
			operator=(std::move(other));
		}

		DenseMap& operator=(DenseMap&& other) {
			std::swap(_entries, other._entries);
			std::swap(_size, other._size);
			return *this;
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

		bool contains(const Key& key) const {
			return find_key(key);
		}

		iterator find(const Key& key) {
			Entry* entry = const_cast<Entry*>(find_key(key));
			return make_iterator(entry ? entry : _entries.end());
		}

		const_iterator find(const Key& key) const {
			const Entry* entry = find_key(key);
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
		std::pair<iterator, bool> emplace(const Key& key, Args&&... args) {
			y_defer(audit());

			if(should_expand()) {
				expand();
			}

			y_debug_assert(!should_expand());

			const usize index = find_bucket_index_for_insert(key);
			const bool exists = _entries[index].has_key();

			if(!exists) {
				_entries[index].emplace(key, y_fwd(args)...);
				++_size;
			}

			return {make_iterator(&_entries[index]), !exists};
		}

		std::pair<iterator, bool> insert(std::pair<const Key, Value> p) {
			return emplace(p.first, std::move(p.second));
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

using namespace external;
using namespace stable;
using namespace compact;

}
}

#endif // Y_CORE_DENSEMAP_H
