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
#ifndef Y_MEM_MEMORY_H
#define Y_MEM_MEMORY_H

#include <y/utils.h>
#include <cstddef>

namespace y {
namespace memory {

constexpr usize max_alignment = std::alignment_of<std::max_align_t>::value;

constexpr usize align_up_to(usize value, usize alignment) {
	if(usize diff = value % alignment) {
		return value + alignment - diff;
	}
	return value;
	//return (value + alignment - 1) & ~(alignment - 1);
}

constexpr usize align_down_to(usize value, usize alignment) {
	usize diff = value % alignment;
	return value - diff;
}

constexpr usize align_up_to_max(usize size) {
	return align_up_to(size, max_alignment);
}

class PolymorphicAllocatorBase : NonCopyable{
	public:
		virtual ~PolymorphicAllocatorBase() {
		}

		[[nodiscard]] virtual void* allocate(usize size) noexcept = 0;
		virtual void deallocate(void* ptr, usize size) noexcept = 0;
};

class PolymorphicAllocatorContainer : NonCopyable {
	public:
		// does NOT take ownership
		PolymorphicAllocatorContainer(NotOwner<PolymorphicAllocatorBase*> allocator) : _inner(allocator) {
		}

		PolymorphicAllocatorContainer(PolymorphicAllocatorContainer&& other) : _inner(other._inner) {
		}

		[[nodiscard]] void* allocate(usize size) noexcept {
			return _inner->allocate(size);
		}

		void deallocate(void* ptr, usize size) noexcept {
			return _inner->deallocate(ptr, size);
		}

	private:
		NotOwner<PolymorphicAllocatorBase*> _inner;
};

template<typename Allocator>
class PolymorphicAllocator : public PolymorphicAllocatorBase, Allocator {
	public:
		using Allocator::Allocator;

		[[nodiscard]] void* allocate(usize size) noexcept override {
			return Allocator::allocate(size);
		}

		void deallocate(void* ptr, usize size) noexcept override {
			Allocator::deallocate(ptr, size);
		}
};


PolymorphicAllocatorBase& global_allocator();
PolymorphicAllocatorBase& thread_local_allocator();

}
}

#endif // Y_MEM_MEMORY_H
