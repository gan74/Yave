/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
#ifndef YAVE_ECS_FREELISTNODE_H
#define YAVE_ECS_FREELISTNODE_H

#include <y/utils.h>
#include <yave/yave.h>

namespace yave {
namespace ecs {

template<typename Tag>
union FreeListId {
	public:
		FreeListId() {
			parts.index = invalid_index;
			parts.version = 0;
		}

		bool operator==(FreeListId i) const {
			return i.id == id;
		}

		bool operator!=(FreeListId i) const {
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
		friend class FreeListNode;

		static constexpr u32 invalid_index = u32(-1);

		struct {
			u32 index;
			u32 version;
		} parts;
		u64 id;
};

template<typename T, typename IDTag = T>
class FreeListNode {
	public:
		using Id = FreeListId<IDTag>;
		static constexpr u32 invalid_index = Id::invalid_index;

		static_assert(std::is_trivially_copyable_v<Id>);

		FreeListNode() {
		}

		FreeListNode(const FreeListNode& other) : _id(other._id) {
			if(is_free()) {
				_storage.next = other._storage.next;
			} else {
				_storage.obj = other._storage.obj;
			}
		}

		FreeListNode(FreeListNode&& other) : _id(other._id) {
			if(is_free()) {
				_storage.next = other._storage.next;
			} else {
				_storage.obj = std::move(other._storage.obj);
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


}
}

#endif // YAVE_ECS_FREELISTNODE_H
