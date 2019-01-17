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

namespace yave {
namespace ecs {

template<typename Tag>
union SlotMapId {
	public:
		SlotMapId() {
			parts.index = invalid_index;
			parts.version = 0;
		}

		bool operator==(SlotMapId i) const {
			return i.id == id;
		}

		bool operator!=(SlotMapId i) const {
			return i.id != id;
		}

		bool is_valid() const {
			return parts.index != invalid_index;
		}

		u32 index() const {
			return parts.index;
		}

		u32 version() const {
			return parts.version;
		}

	private:
		template<typename T, typename IDTag>
		friend class SlotMap;

		static constexpr u32 invalid_index = u32(-1);

		struct {
			u32 index;
			u32 version;
		} parts;
		u64 id;
};

template<typename T, typename IDTag = T>
class SlotMap {
	class FreeListNode {
		public:
			using Id = SlotMapId<IDTag>;
			static constexpr u32 invalid_index = Id::invalid_index;

			static_assert(std::is_trivially_copyable_v<Id>);

			FreeListNode() {
			}

			FreeListNode(const FreeListNode& other) : _id(other._id) {
				if(is_free()) {
					_storage.next = other._storage.next;
				} else {
					new(&_storage.obj) T(other._storage.obj);
				}
			}

			FreeListNode(FreeListNode&& other) : _id(other._id) {
				if(is_free()) {
					_storage.next = other._storage.next;
				} else {
					new(&_storage.obj) T(std::move(other._storage.obj));
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
				return _id.parts.index == invalid_index;
			}

			u32 next_free() const {
				y_debug_assert(is_free());
				return _storage.next;
			}

			bool has_next() const {
				y_debug_assert(is_free());
				return _storage.next != invalid_index;
			}

			void create(u32 index) {
				y_debug_assert(is_free());
				_id.parts.index = index;
				++_id.parts.version;
				new(&_storage.obj) T();
				y_debug_assert(!is_free());
			}

			u32 destroy(u32 next = invalid_index) {
				y_debug_assert(!is_free());
				u32 last_index = _id.parts.index;
				_id.parts.index = invalid_index;
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
				return std::pair<Id, T&>(_id, get());
			}

		private:
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

	template<bool Const>
	class Iterator {
		using internal_t = std::conditional_t<Const, const node_t*, node_t*>;
		public:
			using reference = decltype(std::declval<internal_t>()->get());
			using pointer = decltype(&std::declval<internal_t>()->get());
			using value_type = std::remove_reference_t<reference>;
			using difference_type = usize;
			using iterator_category = std::forward_iterator_tag;

			static_assert(std::is_reference_v<reference>);
			static_assert(std::is_pointer_v<pointer>);
			static_assert(!std::is_reference_v<pointer>);

			Iterator(internal_t it  = nullptr) : _it(it) {
				skip_emtpy();
			}

			Iterator& operator++() {
				++_it;
				skip_emtpy();
				return *this;
			}

			Iterator& operator++(int) {
				auto a = *this;
				++(*this);
				return a;
			}

			auto& operator*() const {
				return _it->get();
			}

			auto operator->() const {
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

		static constexpr u32 invalid_index = node_t::invalid_index;


		// We need the last element of the _nodes vector to be an empty node to ensure that an iterator can not run past the end
		SlotMap() {
			_nodes.emplace_back();
		}

		Id add() {
			y_debug_assert(_nodes.last().is_free());

			u32 index = _next;
			if(index == invalid_index) {
				// account for extra end node, and add a new one
				index = _nodes.size() - 1;
				_nodes.emplace_back();
			}
			node_t& node = _nodes[index];
			_next = node.next_free();
			node.create(index);
			y_debug_assert(_nodes.last().is_free());
			return node.id();
		}

		void remove(Id id) {
			y_debug_assert(_nodes.last().is_free());

			if(id.index() >= _nodes.size()) {
				return;
			}
			node_t& node = _nodes[id.index()];
			if(node.id() != id) {
				return;
			}
			_next = node.destroy(_next);
		}

		T* get(Id id) {
			if(id.index() >= _nodes.size()) {
				return nullptr;
			}
			node_t& node = _nodes[id.index()];
			if(node.id() != id) {
				return nullptr;
			}
			return &node.get();
		}

		const T* get(Id id) const{
			return const_cast<std::remove_const_t<decltype(this)>>(this)->get(id);
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

	private:
		core::Vector<node_t> _nodes;
		u32 _next = invalid_index;
};

}
}

#endif // YAVE_ECS_SLOTMAP_H
