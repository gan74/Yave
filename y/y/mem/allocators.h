/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#ifndef Y_MEM_ALLOCATORS_H
#define Y_MEM_ALLOCATORS_H

#include "memory.h"

#include <y/utils/format.h>

#include <algorithm>
#include <mutex>

Y_TODO(Remove and use PMR)

namespace y {
namespace memory {

// -------------------------- obvious ones --------------------------

class NullAllocator : NonCopyable {
	public:
		[[nodiscard]] void* allocate(usize) noexcept {
			return nullptr;
		}

		void deallocate(void* ptr, usize) noexcept {
			y_always_assert(!ptr, "nullptr expected");
		}
};

class Mallocator : NonCopyable {
	public:
		[[nodiscard]] void* allocate(usize size) noexcept {
			return std::malloc(align_up_to_max(size));
		}

		void deallocate(void* ptr, usize size) noexcept {
			unused(size);
			std::free(ptr);
		}
};

// -------------------------- polymorphic allocators --------------------------

class PolymorphicAllocatorBase : NonCopyable {
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

		PolymorphicAllocatorContainer(PolymorphicAllocatorContainer&&) = default;
		PolymorphicAllocatorContainer& operator=(PolymorphicAllocatorContainer&&) = default;

		PolymorphicAllocatorBase* allocator() {
			return _inner;
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
class PolymorphicAllocator : public PolymorphicAllocatorBase {
	public:
		PolymorphicAllocator() = default;
		PolymorphicAllocator(Allocator&& a) : _allocator(std::move(a)) {
		}

		[[nodiscard]] void* allocate(usize size) noexcept override {
			return _allocator.allocate(size);
		}

		void deallocate(void* ptr, usize size) noexcept override {
			_allocator.deallocate(ptr, size);
		}

	private:
		Allocator _allocator;
};

// -------------------------- compound allocators --------------------------

template<typename Allocator>
class ThreadSafeAllocator : NonCopyable {
	public:
		ThreadSafeAllocator() = default;

		ThreadSafeAllocator(Allocator&& a) : _allocator(std::move(a)) {
		}

		[[nodiscard]] void* allocate(usize size) noexcept {
			const std::unique_lock lock(_lock);
			return _allocator.allocate(size);
		}

		void deallocate(void* ptr, usize size) noexcept {
			const std::unique_lock lock(_lock);
			_allocator.deallocate(ptr, size);
		}

	private:
		Allocator _allocator;
		std::mutex _lock;
};

template<usize Threshold, typename Small, typename Large>
class SegregatorAllocator : NonCopyable {
	public:

		SegregatorAllocator() = default;

		SegregatorAllocator(Small&& a) : _small(std::move(a)) {
		}

		SegregatorAllocator(Small&& a, Large&& f) : _small(std::move(a)), _large(std::move(f)) {
		}

		[[nodiscard]] void* allocate(usize size) noexcept {
			if(align_up_to_max(size) <= Threshold) {
				return _small.allocate(size);
			}
			return _large.allocate(size);
		}

		void deallocate(void* ptr, usize size) noexcept {
			if(align_up_to_max(size) <= Threshold) {
				_small.deallocate(ptr, size);
			} else {
				_large.deallocate(ptr, size);
			}
		}

	private:
		Small _small;
		Large _large;
};

template<typename Allocator>
class ElectricFenceAllocator : NonCopyable {
	public:
		static constexpr usize fence_size = align_up_to_max(1024);
		static constexpr u8 fence = 0xFE;

		ElectricFenceAllocator() = default;

		ElectricFenceAllocator(Allocator&& a) : _allocator(std::move(a)) {
		}

		[[nodiscard]] void* allocate(usize size) noexcept {
			u8* f_begin = static_cast<u8*>(_allocator.allocate(2 * fence_size + size));
			if(f_begin) {
				u8* f_end = f_begin + fence_size;
				u8* s_begin = f_end + size;
				u8* s_end = s_begin + fence_size;
				std::fill(f_begin, f_end, fence);
				std::fill(s_begin, s_end, fence);
				return f_end;
			}
			return f_begin;
		}

		void deallocate(void* ptr, usize size) noexcept {
			const u8* f_end = static_cast<u8*>(ptr);
			const u8* f_begin = f_end - fence_size;
			if(ptr) {
				u8* s_begin = f_end + size;
				u8* s_end = s_begin + fence_size;
				const auto is_fence = [](u8 c) { return c == fence; };
				if(std::find_if_not(f_begin, f_end, is_fence) != f_end ||
				   std::find_if_not(s_begin, s_end, is_fence) != s_end) {
					y_fatal("Fence altered: buffer overflow detected (alloc size: %).", size);
				}
			}
			_allocator.deallocate(f_begin, 2 * fence_size + size);
		}

	private:
		Allocator _allocator;
};

template<typename Allocator>
class LeakDetectorAllocator : NonCopyable {
	public:
		LeakDetectorAllocator() = default;

		LeakDetectorAllocator(Allocator&& a) : _allocator(std::move(a)) {
		}

		~LeakDetectorAllocator() {
			if(_alive) {
				y_fatal("Memory was not freed before allocator destruction (% bytes leaked).", _alive);
			}
		}

		[[nodiscard]] void* allocate(usize size) noexcept {
			_alive += size;
			return _allocator.allocate(size);
		}

		void deallocate(void* ptr, usize size) noexcept {
			if(size > _alive) {
				y_fatal("More memory was freed than has been allocated (currently allocated: % bytes, trying to free % bytes).", _alive, size);
			}
			_alive -= size;
			_allocator.deallocate(ptr, size);
		}

	private:
		Allocator _allocator;
		usize _alive = 0;
};



}
}


#endif // Y_MEM_ALLOCATORS_H
