/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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
#ifndef Y_CORE_SMALLVECTOR_H
#define Y_CORE_SMALLVECTOR_H

#include "Vector.h"

namespace y {
namespace core {

namespace detail {

constexpr usize max(usize a, usize b) {
	return a < b ? b : a;
}

template<typename T>
static constexpr usize small_vector_default_size = max(1, 64 - sizeof(Vector<int>) / sizeof(T));

}

template<typename Elem, usize Size, typename Allocator = std::allocator<Elem>>
class SmallVectorAllocator : Allocator {

	using data_type = typename std::remove_const<Elem>::type;

	public:
		using value_type = Elem;
		using propagate_on_container_move_assignment = typename Allocator::propagate_on_container_move_assignment;

		value_type* allocate(usize size, const void* hint = nullptr) {
			if(size <= Size) {
				return _storage;
			}
			return Allocator::allocate(size, hint);
		}

		void deallocate(value_type* ptr, usize size) {
			if(size > Size) {
				Allocator::deallocate(ptr, size);
			}
		}

		bool operator==(const SmallVectorAllocator& other) const {
			return Allocator::operator==(other);
		}

	private:
		union {
			data_type _storage[Size];
		};
};

template<usize Size, typename ResizePolicy = DefaultVectorResizePolicy>
class SmallVectorResizePolicy : ResizePolicy {
	public:
		usize ideal_capacity(usize size) const {
			if(size <= Size) {
				return Size;
			}
			return size <= 4 * Size ? 4 * Size : ResizePolicy::ideal_capacity(size);
		}

		bool shrink(usize size, usize capacity) const {
			return capacity <= Size ? false : ResizePolicy::shrink(size, capacity);
		}
};


template<typename Elem, usize Size = detail::small_vector_default_size<Elem>, typename ResizePolicy = DefaultVectorResizePolicy, typename Allocator = std::allocator<Elem>>
using SmallVector = Vector<Elem, SmallVectorResizePolicy<Size, ResizePolicy>, SmallVectorAllocator<Elem, Size, Allocator>>;


}
}

#endif // Y_CORE_SMALLVECTOR_H
