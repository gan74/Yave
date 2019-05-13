/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#ifndef Y_CORE_SLOTMAP_H
#define Y_CORE_SLOTMAP_H

#include "Vector.h"
#include "Range.h"
#include "Result.h"

namespace y {
namespace core {

template<typename T>
class SlotMapKey {
	public:
		SlotMapKey() {
			_parts.index = invalid_index;
			_parts.version = 0;
		}

		bool operator==(SlotMapKey i) const {
			return i._id == _id;
		}

		bool operator!=(SlotMapKey i) const {
			return i._id != _id;
		}

		bool operator<(SlotMapKey i) const {
			return std::tie(_parts.index, _parts.version) < std::tie(i._parts.index, i._parts.version);
		}

		bool operator>(SlotMapKey i) const {
			return std::tie(_parts.index, _parts.version) > std::tie(i._parts.index, i._parts.version);
		}

		u32 index() const {
			return _parts.index;
		}

		u32 version() const {
			return _parts.version;
		}

		bool is_null() const {
			return _parts.index == invalid_index;
		}

		u64 numeric_id() const {
			return _id;
		}

		static SlotMapKey from_numeric_id(u64 id) {
			SlotMapKey key;
			key._id = id;
			return key;
		}

	private:
		template<typename U, typename V>
		friend class SlotMap;

		void set_null() {
			_parts.index = invalid_index;
			++_parts.version;
		}

		void set(u32 index) {
			_parts.index = index;
		}

		void set_version(u32 v) {
			_parts.version = v;
		}

		static constexpr u32 invalid_index = u32(-1);

		union {
			struct {
				u32 index;
				u32 version;
			} _parts;
			u64 _id;
		};
};

template<typename T, typename Tag = T>
class SlotMap {
	public:
		using key_type = SlotMapKey<Tag>;
		using mapped_type = T;
		using value_type = std::pair<const key_type, mapped_type>;


	private:
		class Bucket : NonCopyable {
			public:
				Bucket() = default;

				~Bucket() {
					clear();
				}

				Bucket(Bucket&& other) {
					swap(other);
				}

				Bucket& operator=(Bucket&& other) {
					swap(other);
					return *this;
				}

				void clear() {
					if(!is_empty()) {
						_key.set_null();
						_storage.clear();
					}
					y_debug_assert(is_empty());
				}

				template<typename... Args>
				void fill(u32 index, Args&&... args) {
					clear();
					_key.set(index);
					_storage.set(y_fwd(args)...);
				}

				bool increase_version(u32 version) {
					if(version >= _key.version()) {
						_key.set_version(version);
						return true;
					}
					return false;
				}




				bool is_empty() const {
					return _key.is_null();
				}

				key_type key() const {
					return _key;
				}

				bool matches_key(key_type key) const {
					return _key == key;
				}



				T& value() {
					y_debug_assert(!is_empty());
					return _storage.value;
				}

				const T& value() const {
					y_debug_assert(!is_empty());
					return _storage.value;
				}

				std::pair<key_type, mapped_type&> as_pair() {
					return {this->key(), this->value()};
				}

				std::pair<key_type, const mapped_type&> as_pair() const {
					return {this->key(), this->value()};
				}




				usize next_full_index() const {
					y_debug_assert(is_empty());
					return usize(_storage.free_list.next_full_or_block_begin);
				}



				auto& free_list() {
					y_debug_assert(is_empty());
					return _storage.free_list;
				}

				const auto& free_list() const {
					y_debug_assert(is_empty());
					return _storage.free_list;
				}

				static bool is_valid_index(usize index) {
					return index != invalid_index();
				}

				static usize invalid_index() {
					return usize(Storage::invalid_index);
				}

			private:
				key_type _key;
				union Storage {
					static constexpr u32 invalid_index = u32(-1);

					Storage() : free_list({invalid_index, invalid_index, invalid_index}) {
					}

					~Storage() {
					}

					template<typename... Args>
					void set(Args&&... args) {
						::new(&value) mapped_type(y_fwd(args)...);
					}

					void clear() {
						value.~mapped_type();
					}

					struct {
						u32 next_full_or_block_begin;
						u32 next_free_block;
						u32 prev_free_block;
					} free_list;

					mapped_type value;
				} _storage;

				void swap(Bucket& other) {
					if(is_empty() && !other.is_empty()) {
						other.swap(*this);
					} else if(!is_empty() && other.is_empty()) { // oh god...
						std::swap(_key, other._key);
						auto free = other._storage.free_list;
						new(&other._storage.value) mapped_type(std::move(_storage.value));
						_storage.value.~mapped_type();
						_storage.free_list = free;
					} else { // same state
						if(is_empty()) {
							std::swap(_storage.free_list, other._storage.free_list);
						} else {
							std::swap(_storage.value, other._storage.value);
						}
					}
				}

		};

		struct EndIterator {};

		template<bool Const, typename CRTP>
		class Iterator {
			using bucket_type = std::conditional_t<Const, const Bucket*, Bucket*>;

			public:
				using difference_type = usize;

				bool at_end() const {
					return !Bucket::is_valid_index(_index);
				}

				CRTP& operator++() {
					find_next();
					return static_cast<CRTP&>(*this);
				}

				CRTP operator++(int) {
					CRTP a = static_cast<CRTP>(*this);
					++(*this);
					return a;
				}

				bool operator==(const CRTP& other) const {
					return _index == other._index;
				}

				bool operator!=(const CRTP& other) const {
					return _index != other._index;
				}

				bool operator<(const CRTP& other) const {
					return _index < other._index;
				}

				bool operator>(const CRTP& other) const {
					return _index > other._index;
				}

			protected:
				friend class SlotMap;

				Iterator() : _buckets(nullptr), _index(Bucket::invalid_index()) {
				}

				Iterator(bucket_type begin) : Iterator(begin, begin) {
				}

				Iterator(bucket_type begin, bucket_type self) :
						_buckets(begin),
						_index(self - begin) {

					if(current_bucket()->is_empty()) {
						find_next();
					}
				}

				bucket_type current_bucket() const {
					return _buckets + _index;
				}

			private:
				void find_next() {
					++_index;
					if(current_bucket()->is_empty()) {
						_index = current_bucket()->next_full_index();
					}
				}

				bucket_type _buckets;
				usize _index;
		};

		template<bool Const>
		class PairIterator : public Iterator<Const, PairIterator<Const>> {
			using mapped_reference_type = std::conditional_t<Const, const mapped_type&, mapped_type&>;
			public:
				using value_type = std::pair<key_type, mapped_reference_type>;
				using reference = value_type;
				using iterator_category = std::input_iterator_tag;

				value_type operator*() const {
					return this->current_bucket()->as_pair();
				}

				using Iterator<Const, PairIterator<Const>>::Iterator;
		};

		class KeyIterator : public Iterator<true, KeyIterator> {
			public:
				using value_type = key_type;
				using reference = key_type;
				using iterator_category = std::input_iterator_tag;

				value_type operator*() const {
					return this->current_bucket()->key();
				}

				using Iterator<true, KeyIterator>::Iterator;
		};

		template<bool Const>
		class ValueIterator : public Iterator<Const, ValueIterator<Const>> {
			public:
				using value_type = mapped_type;
				using reference = std::conditional_t<Const, const mapped_type&, mapped_type&>;
				using pointer = std::conditional_t<Const, const mapped_type*, mapped_type*>;
				using iterator_category = std::forward_iterator_tag;

				reference operator*() const {
					return this->current_bucket()->value();
				}

				pointer operator->() const {
					return &this->current_bucket()->value();
				}

				using Iterator<Const, ValueIterator<Const>>::Iterator;
		};

	public:
		using iterator = ValueIterator<false>;
		using const_iterator = ValueIterator<false>;


		iterator begin() {
			return iterator(_buckets.begin());
		}

		iterator end() {
			return iterator();
		}

		const_iterator begin() const {
			return iterator(_buckets.begin());
		}

		const_iterator end() const {
			return const_iterator();
		}


		auto values() {
			return Range(begin(), end());
		}

		auto values() const {
			return Range(begin(), end());
		}

		auto keys() const {
			return Range(KeyIterator(_buckets.begin()), KeyIterator());
		}

		auto pairs() {
			return Range(PairIterator<false>(_buckets.begin()), PairIterator<false>());
		}

		auto pairs() const {
			return Range(PairIterator<true>(_buckets.begin()), PairIterator<true>());
		}





		SlotMap() {
			// guard bucket
			_buckets.emplace_back();
			y_debug_assert(_buckets.first().is_empty());
			audit();
		}

		mapped_type* get(key_type key) {
			return contains(key) ? &_buckets[key.index()].value() : nullptr;
		}

		const mapped_type* get(key_type key) const {
			return contains(key) ? &_buckets[key.index()].value() : nullptr;
		}

		mapped_type& operator[](key_type key) {
			y_debug_assert(contains(key));
			return _buckets[key.index()].value();
		}

		const mapped_type& operator[](key_type key) const {
			y_debug_assert(contains(key));
			return _buckets[key.index()].value();
		}


		bool contains(key_type key) const {
			return is_valid(key);
		}

		bool is_empty() const {
			return !size();
		}

		usize size() const {
			return _live_values;
		}

		usize capacity() const {
			return _buckets.capacity() - 1;
		}

		void set_min_capacity(usize min_cap) {
			return _buckets.set_min_capacity(min_cap + 1);
		}

		void clear() {
			*this = SlotMap();
		}



		template<typename... Args>
		key_type insert(Args&&... args) {
			auto [index, bucket] = take_empty_bucket();
			bucket.fill(index, y_fwd(args)...);
			++_live_values;
			audit();
			return bucket.key();
		}

		void erase(key_type key) {
			if(contains(key)) {
				free_bucket(key.index());
				--_live_values;
			}
			y_debug_assert(!contains(key));
			audit();
		}

		template<typename... Args>
		Result<mapped_type&> insert_at_index(key_type key, Args&&... args) {
			y_defer(audit());

			const u32 index = key.index();
			auto insert_at_head = [=] {
					y_debug_assert(_free_head == index);
					insert(y_fwd(args)...);
					if(!_buckets[index].set_version(key.version())) {
						return Err();
					}
					y_debug_assert(_buckets[index].key() == key);
					return Ok(operator[](key));
				};

			// fast path
			if(index >= _buckets.size()) {
				while(_free_head != index) {
					add_bucket();
				}
				return insert_at_head();
			}

			if(!_buckets[index].is_empty()) {
				return Err();
			}

			if(_free_head == index) {
				insert_at_head();
			}

			for(usize it = _free_head; Bucket::is_valid_index(it); it = _buckets[it].next_free_index()) {
				if(_buckets[it].next_free_index() == index) {
					/*auto& free_list = _buckets[it].free_list;

					_buckets[index].set_next_free_index(_free_head);
					_free_head = index;
					return insert_at_head();*/
				}
			}
			return y_fatal("Unable to find key.");
		}

	private:
		void add_bucket() {
			y_debug_assert(_free_head >= _buckets.size() - 1);
			_free_head = _buckets.size() - 1;
			_buckets.emplace_back();
		}

		std::pair<usize, Bucket&> take_empty_bucket() {
			if(_free_head >= _buckets.size() - 1) {
				add_bucket();
			}

			Bucket& bucket = _buckets[_free_head];
			auto& free_list = bucket.free_list();

			y_debug_assert(bucket.is_empty());

			const usize index = _free_head;
			// 1 size block
			if(free_list.next_full_or_block_begin == index + 1)  {
				_free_head = free_list.next_free_block;
			} else {
				// block at end
				if(!Bucket::is_valid_index(free_list.next_full_or_block_begin)) {
					_free_head = index + 1;
					_buckets[_free_head].free_list().next_full_or_block_begin = Bucket::invalid_index();
				} else {
					Bucket& last_bucket = _buckets[free_list.next_full_or_block_begin - 1];
					y_debug_assert(last_bucket.free_list().next_full_or_block_begin == index);
					_free_head = ++last_bucket.free_list().next_full_or_block_begin;
				}
			}
			_buckets[_free_head].free_list().prev_free_block = Bucket::invalid_index();

			return {index, bucket};
		}

		void free_bucket(usize index) {
			Bucket& bucket = _buckets[index];
			y_debug_assert(!bucket.is_empty());
			bucket.clear();

			if(Bucket& next_bucket = _buckets[index + 1]; next_bucket.is_empty()) {
				if(index && _buckets[index - 1].is_empty()) {
					Y_TODO(merge the two blocks)
					y_debug_assert(false);
				} else {
					auto& free_list = next_bucket.free_list();

					bucket.free_list().prev_free_block = free_list.prev_free_block;
					bucket.free_list().next_full_or_block_begin = free_list.next_full_or_block_begin;

					if(Bucket::is_valid_index(free_list.prev_free_block)) {
						_buckets[free_list.prev_free_block].free_list().next_free_block = index;
					} else {
						y_debug_assert(_free_head == index + 1);
						_free_head = index;
					}

					if(Bucket::is_valid_index(free_list.next_full_or_block_begin)) {
						Bucket& end = _buckets[free_list.next_full_or_block_begin - 1];
						y_debug_assert(end.free_list().next_full_or_block_begin == index + 1);
						end.free_list().next_full_or_block_begin = index;
					}
				}
			} else if(index && _buckets[index - 1].is_empty()) {
				Bucket& prev_bucket = _buckets[index - 1];
				auto& free_list = prev_bucket.free_list();

				bucket.free_list().next_full_or_block_begin = free_list.next_full_or_block_begin;
				y_debug_assert(Bucket::is_valid_index(free_list.next_full_or_block_begin));
				Bucket& begin = _buckets[free_list.next_full_or_block_begin];
				y_debug_assert(begin.free_list().next_full_or_block_begin == index);
				begin.free_list().next_full_or_block_begin = index + 1;
			} else {
				y_debug_assert(index == 0 || !_buckets[index - 1].is_empty());
				y_debug_assert(!_buckets[index + 1].is_empty());
				auto& free_list = bucket.free_list();
				free_list.next_full_or_block_begin = index + 1;
				free_list.next_free_block = _free_head;
				free_list.prev_free_block = Bucket::invalid_index();
				_free_head = index;
			}
		}

		bool is_valid(key_type key) const {
			return !key.is_null() &&
				   key.index() < _buckets.size() - 1 &&
				   _buckets[key.index()].matches_key(key);
		}

		void audit() const {
#ifdef Y_AUDIT_SLOTMAP
			y_debug_assert(_buckets.size() > 0);
			y_debug_assert(_buckets.last().is_empty());
			{
				usize size = 0;
				for(const Bucket& b : _buckets) {
					size += !b.is_empty();
				}
				y_debug_assert(size == _live_values);
			}
			{
				for(usize i = 0; i != _buckets.size() - 1; ++i) {
					y_debug_assert(_buckets[i].is_empty() || _buckets[i].key().index() == i);
				}
			}
			{
				usize empties = _buckets.size() - _live_values;
				usize real_empties = 0;
				usize it = _free_head;
				while(Bucket::is_valid_index(it)) {
					const Bucket& bucket = _buckets[it];
					usize block_end = bucket.free_list().next_full_or_block_begin;
					for(usize i = it; i < block_end && i < _buckets.size(); ++i) {
						++real_empties;
						y_debug_assert(_buckets[i].is_empty());
					}
					it = bucket.free_list().next_free_block;
				}
				y_debug_assert(real_empties == empties);
			}
			{

			}
#endif
		}


		core::Vector<Bucket> _buckets;
		usize _live_values = 0;
		usize _free_head = 0;

};


}
}

#endif // Y_CORE_SLOTMAP_H
