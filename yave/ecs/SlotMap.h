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
#ifndef YAVE_ECS_SLOTMAP_H
#define YAVE_ECS_SLOTMAP_H

#include <y/core/Vector.h>
#include <yave/yave.h>

#include <y/core/Result.h>

namespace yave {
namespace ecs {

template<typename Tag>
class SlotMapId {
	public:
		SlotMapId() {
			_parts.index = invalid_index;
			_parts.version = 0;
		}

		static SlotMapId from_full_id(u64 id) {
			SlotMapId i;
			i._id = id;
			return i;
		}

		bool operator==(SlotMapId i) const {
			return i._id == _id;
		}

		bool operator!=(SlotMapId i) const {
			return i._id != _id;
		}

		bool operator<(SlotMapId i) const {
			return std::tie(_parts.index, _parts.version) < std::tie(i._parts.index, i._parts.version);
		}


		bool is_valid() const {
			return _parts.index != invalid_index;
		}

		u32 index() const {
			return _parts.index;
		}

		u32 version() const {
			return _parts.version;
		}

		u64 full_id() const {
			return _id;
		}

	private:
		template<typename T, typename IDTag>
		friend class SlotMap;

		static constexpr u32 invalid_index = u32(-1);

		union {
			struct {
				u32 index;
				u32 version;
			} _parts;
			u64 _id;
		};
};

template<typename T, typename IDTag = T>
class SlotMap {
	class FreeListNode : NonCopyable {
		public:
			using Id = SlotMapId<IDTag>;
			static constexpr u32 invalid_index = Id::invalid_index;

			static_assert(std::is_trivially_copyable_v<Id>);

			FreeListNode() {
			}

			FreeListNode(FreeListNode&& other) : _id(other._id) {
				if(is_free()) {
					_storage.next = other._storage.next;
				} else {
					::new(&_storage.obj) T(std::move(other._storage.obj));
				}
			}

			~FreeListNode() {
				if(!is_free()) {
					destroy();
				}
			}

			Id id() const {
				return _id;
			}

			bool is_free() const {
				return _id._parts.index == invalid_index;
			}

			u32 next_free() const {
				y_debug_assert(is_free());
				return _storage.next;
			}

			bool has_next() const {
				y_debug_assert(is_free());
				return _storage.next != invalid_index;
			}

			template<typename... Args>
			void create(u32 index, Args&&... args) {
				y_debug_assert(is_free());
				_id._parts.index = index;
				++_id._parts.version;
				new(&_storage.obj) T(y_fwd(args)...);
				y_debug_assert(!is_free());
			}

			u32 destroy(u32 next = invalid_index) {
				y_debug_assert(!is_free());
				u32 last_index = _id._parts.index;
				_id._parts.index = invalid_index;
				_storage.obj.~T();
				_storage.next = next;
				return last_index;
			}

			T& get() {
				y_debug_assert(!is_free());
				return _storage.obj;
			}

			const T& get() const {
				y_debug_assert(!is_free());
				return _storage.obj;
			}

			std::pair<Id, T&> as_pair() {
				return std::pair<Id, T&>(_id, get());
			}

			std::pair<Id, const T&> as_pair() const {
				return std::pair<Id, const T&>(_id, get());
			}

		private:
			friend class SlotMap;

			Id _id;
			union Storage {
				u32 next;
				T obj;

				Storage() : next(invalid_index) {
				}

				~Storage() {
				}
			} _storage;
	};


	using node_t = FreeListNode;

	template<bool Const, bool Pairs = false>
	class Iterator {
		using internal_t = std::conditional_t<Const, const node_t*, node_t*>;
		public:
			using reference = std::conditional_t<Pairs, decltype(std::declval<internal_t>()->as_pair()), decltype(std::declval<internal_t>()->get())>;
			using pointer = decltype(&std::declval<internal_t>()->get());
			using value_type = std::remove_reference_t<reference>;
			using difference_type = usize;
			using iterator_category = std::forward_iterator_tag;

			static_assert(std::is_reference_v<reference> == !Pairs);
			static_assert(std::is_pointer_v<pointer>);
			static_assert(!std::is_reference_v<pointer>);

			Iterator(internal_t it  = nullptr) : _it(it) {
				if(it) {
					skip_emtpy();
				}
			}

			Iterator& operator++() {
				++_it;
				skip_emtpy();
				return *this;
			}

			Iterator operator++(int) {
				auto a = *this;
				++(*this);
				return a;
			}

			reference operator*() const {
				if constexpr(Pairs) {
					return _it->as_pair();
				} else {
					return _it->get();
				}
			}

			pointer operator->() const {
				static_assert(!Pairs);
				return &_it->get();
			}

			bool operator==(const Iterator& other) const {
				return _it == other._it;
			}

			bool operator!=(const Iterator& other) const {
				return _it != other._it;
			}

			bool operator<(const Iterator& other) const {
				return _it < other._it;
			}

			bool operator>(const Iterator& other) const {
				return _it > other._it;
			}

		private:
			void skip_emtpy() {
				while(_it->is_free()) {
					++_it;
				}
			}

			internal_t _it;
	};

	public:
		using Id = typename node_t::Id;

		using iterator = Iterator<false>;
		using const_iterator = Iterator<true>;

		using pair_iterator = Iterator<false, true>;
		using const_pair_iterator = Iterator<true, true>;

		static constexpr u32 invalid_index = node_t::invalid_index;


		// We need the last element of the _nodes vector to be an empty node to ensure that an iterator can not run past the end
		SlotMap() {
			_nodes.emplace_back();
		}

		template<typename... Args>
		Id insert(Args&&... args) {
			y_debug_assert(_nodes.last().is_free());

			u32 index = _next;
			if(index == invalid_index) {
				// account for extra end node, and add a new one
				index = _nodes.size() - 1;
				_nodes.emplace_back();
			}
			node_t& node = _nodes[index];
			_next = node.next_free();
			node.create(index, y_fwd(args)...);
			y_debug_assert(_nodes.last().is_free());
			return node.id();
		}

		template<typename... Args>
		core::Result<void> insert_with_id(Id id, Args&&... args) {
			y_debug_assert(_nodes.last().is_free());

			u32 index = id.index();
			if(index < _nodes.size()) {
				if(!_nodes[index].is_free()) {
					return core::Err();
				}
			}

			while(index >= _nodes.size() - 1) {
				_nodes[_nodes.size() - 1]._storage.next = _next;
				_next = _nodes.size() - 1;
				_nodes.emplace_back();
			}

			auto* last = &_next;
			for(auto i = _next; i != invalid_index; i = _nodes[i]._storage.next) {
				if(i == index) {
					node_t& node = _nodes[index];
					*last = node._storage.next;
					node.create(index, y_fwd(args)...);
					return core::Ok();
				}
				last = &_nodes[i]._storage.next;
			}
			return y_fatal("Unable to find index");
		}

		void erase(Id id) {
			y_debug_assert(_nodes.last().is_free());
			y_debug_assert(!_nodes.is_empty());
			if(id.index() >= _nodes.size() - 1) {
				return;
			}
			node_t& node = _nodes[id.index()];
			if(node.id() != id) {
				return;
			}
			_next = node.destroy(_next);
		}

		T* get(Id id) {
			y_debug_assert(!_nodes.is_empty());
			if(id.index() >= _nodes.size() - 1) {
				return nullptr;
			}
			node_t& node = _nodes[id.index()];
			if(node.id() != id) {
				return nullptr;
			}
			return &node.get();
		}

		const T* get(Id id) const {
			y_debug_assert(!_nodes.is_empty());
			if(id.index() >= _nodes.size() - 1) {
				return nullptr;
			}
			const node_t& node = _nodes[id.index()];
			if(node.id() != id) {
				return nullptr;
			}
			return &node.get();
		}

		T& operator[](Id id) {
			T* ptr = get(id);
			y_debug_assert(ptr);
			return *ptr;
		}

		const T& operator[](Id id) const {
			const T* ptr = get(id);
			y_debug_assert(ptr);
			return *ptr;
		}

		iterator begin() {
			return iterator(_nodes.begin());
		}

		iterator end() {
			y_debug_assert(_nodes.last().is_free());
			return iterator(_nodes.end() - 1);
		}

		const_iterator begin() const {
			return const_iterator(_nodes.begin());
		}

		const_iterator end() const {
			y_debug_assert(_nodes.last().is_free());
			return const_iterator(_nodes.end() - 1);
		}

		auto as_pairs() {
			return core::Range(pair_iterator(_nodes.begin()),
							   pair_iterator(_nodes.end() - 1));
		}

		auto as_pairs() const {
			return core::Range(const_pair_iterator(_nodes.begin()),
							   const_pair_iterator(_nodes.end() - 1));
		}

		usize size() const {
			return std::distance(begin(), end());
		}

	private:
		core::Vector<node_t> _nodes;
		u32 _next = invalid_index;
};

}
}

namespace std {
template<typename Tag>
struct hash<yave::ecs::SlotMapId<Tag>> {
	auto operator()(const yave::ecs::SlotMapId<Tag>& p) const {
		auto id = p.full_id();
		return hash<decltype(id)>()(id);
	}
};
}

#endif // YAVE_ECS_SLOTMAP_H
