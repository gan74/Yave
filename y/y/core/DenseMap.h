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

namespace y {
namespace core {

namespace split {
template<typename Key, typename Value, typename Hasher = std::hash<Key>>
class DenseMap : Hasher {

	using index_type = u32;
	static constexpr index_type invalid_index = index_type(-1);

	struct ValueBox {
		Value value;
		index_type key_index;
	};

	struct KeyBox : NonCopyable {
		union {
			Key key;
		};
		index_type value_index = invalid_index;

		KeyBox() = default;

		~KeyBox() {
			make_empty();
		}

		KeyBox(KeyBox&& other) {
			operator=(std::move(other));
		}

		KeyBox& operator=(KeyBox&& other) {
			make_empty();
			if(!other.is_empty()) {
				value_index = other.value_index;
				key = std::move(other.key);
			}
			return *this;
		}

		bool is_empty() const {
			return value_index == invalid_index;
		}

		const Key& get() const {
			y_debug_assert(!is_empty());
			return key;
		}

		void emplace(const Key& k, index_type i) {
			y_debug_assert(is_empty());
			value_index = i;
			new(&key) Key{k};
		}

		void make_empty() {
			if(!is_empty()) {
				value_index = invalid_index;
				key.~Key();
			}
		}

		bool operator==(const Key& k) const {
			return !is_empty() && key == k;
		}
	};

	public:
		static constexpr double max_load_factor = 0.7;

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
			return FilterIterator(_keys.begin(), _keys.end(), [](const KeyBox& box) { return !box.is_empty(); });
		}

		bool should_expand() const {
			return _keys.size() * max_load_factor <=  _values.size();
		}

		usize bucket_index(const Key& key, usize size) const {
			const usize hash = Hasher::operator()(key);
			return hash % size;
		}

		usize bucket_index(const Key& key) const {
			return bucket_index(key, _keys.size());
		}

		void expand() {
			const usize key_count = _keys.size();
			const usize new_size = DefaultVectorResizePolicy::ideal_capacity(key_count + 1);
			y_debug_assert(new_size != key_count);

			auto new_keys = FixedArray<KeyBox>(new_size);
			for(KeyBox& k : _keys) {
				if(!k.is_empty()) {
					const index_type new_index = bucket_index(k.get(), new_size);
					new_keys[new_index].emplace(std::move(k.key), k.value_index);
					_values[k.value_index].key_index = new_index;
				}
			}
			_keys = std::move(new_keys);
		}

		template<typename It>
		bool belongs(const It& it) const {
			const ValueBox* ptr = it.inner();
			return ptr >= _values.data() && ptr < _values.data() + _values.size();
		}

		void audit() const {
#ifdef Y_DEBUG
			for(index_type i = 0; i != _values.size(); ++i) {
				const KeyBox& k = _keys[_values[i].key_index];
				y_debug_assert(!k.is_empty());
				y_debug_assert(k.value_index == i);
			}
			usize key_count = 0;
			for(index_type i = 0; i != _keys.size(); ++i) {
				const KeyBox& k = _keys[i];
				if(k.is_empty()) {
					continue;
				}
				++key_count;
				const ValueBox& v = _values[k.value_index];
				y_debug_assert(v.key_index == i);
				y_debug_assert(bucket_index(k.key) == i);
			}
			y_debug_assert(_values.size() == key_count);
#endif
		}

		FixedArray<KeyBox> _keys;
		Vector<ValueBox> _values;

	public:
		using iterator			= typename decltype(std::declval<      DenseMap<Key, Value>>().key_values())::iterator;
		using const_iterator	= typename decltype(std::declval<const DenseMap<Key, Value>>().key_values())::iterator;

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

		bool contains(const Key& key) const {
			const usize key_index = bucket_index(key);
			return _keys[key_index] == key;
		}

		iterator find(const Key& key) {
			const usize key_index = bucket_index(key);
			const KeyBox& box = _keys[key_index];
			return make_iterator(box == key ? &_values[box.value_index] : _values.end());
		}

		const_iterator find(const Key& key) const {
			const usize key_index = bucket_index(key);
			const KeyBox& box = _keys[key_index];
			return make_iterator(box == key ? &_values[box.value_index] : _values.end());
		}

		template<typename... Args>
		void emplace(const Key& key, Args&&... args) {
			if(should_expand()) {
				expand();
			}

			y_debug_assert(!should_expand());
			const index_type key_index = bucket_index(key);
			const index_type value_index = _values.size();
			_values.emplace_back(ValueBox{Value{y_fwd(args)...}, key_index});
			_keys[key_index].emplace(key, value_index);

			audit();
		}

		void insert(std::pair<const Key, Value> p) {
			emplace(p.first, std::move(p.second));
		}

		void erase(const iterator& it) {
			y_debug_assert(belongs(it));

			ValueBox* value_ptr = it.inner();
			const index_type key_index = value_ptr->key_index;
			KeyBox* key_ptr = &_keys[key_index];
			const index_type value_index = key_ptr->value_index;

			if(value_ptr != &_values.last()) {
				std::swap(*value_ptr, _values.last());
				_keys[_values[value_index].key_index].value_index = value_index;
			}

			_values.pop();
			key_ptr->make_empty();

			audit();
		}
};
}

}
}

#endif // Y_CORE_DENSEMAP_H
