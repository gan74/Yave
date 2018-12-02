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
#ifndef Y_MEM_ALLOCATORS_H
#define Y_MEM_ALLOCATORS_H

#include "memory.h"
#include <mutex>

namespace y {
namespace memory {

namespace detail {

template<typename T>
static auto has_aligned_size(T*) -> bool_type<!std::is_void_v<decltype(T::aligned_size)>>;
template<typename T>
static auto has_aligned_size(...) -> std::false_type;

struct VariableSizeAllocator {
	static constexpr usize aligned_size = 0;
};

template<typename T>
static constexpr bool has_aligned_size_v = decltype(detail::has_aligned_size<T>(nullptr))::value;

}

template<typename T>
static constexpr usize fixed_allocator_size_v = std::conditional_t<detail::has_aligned_size_v<T>, T, detail::VariableSizeAllocator>::aligned_size;

template<typename T>
static constexpr bool is_fixed_size_allocator_v = fixed_allocator_size_v<T> != 0;



// -------------------------- obvious ones --------------------------

class NullAllocator : NonCopyable {
	public:
		[[nodiscard]] void* allocate(usize) noexcept {
			return nullptr;
		}

		void deallocate(void* ptr, usize) noexcept {
			y_debug_assert(!ptr);
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
		ThreadSafeAllocator(ThreadSafeAllocator&&) = default;

		ThreadSafeAllocator(Allocator&& a) : _allocator(std::move(a)) {
		}

		[[nodiscard]] void* allocate(usize size) noexcept {
			std::unique_lock lock(_lock);
			return _allocator.allocate(size);
		}

		void deallocate(void* ptr, usize size) noexcept {
			std::unique_lock lock(_lock);
			_allocator.deallocate(ptr, size);
		}

	private:
		Allocator _allocator;
		std::mutex _lock;
};

template<usize Size, typename Allocator, typename Fallback = NullAllocator>
class FixedSizeAllocator : NonCopyable {
	public:
		static constexpr usize aligned_size = align_up_to_max(Size);

		FixedSizeAllocator() = default;
		FixedSizeAllocator(FixedSizeAllocator&&) = default;

		FixedSizeAllocator(Allocator&& a) : _allocator(std::move(a)) {
		}

		FixedSizeAllocator(Allocator&& a, Fallback&& f) : _allocator(std::move(a)), _fallback(std::move(f)) {
		}

		[[nodiscard]] void* allocate(usize size) noexcept {
			if(align_up_to_max(size) <= aligned_size) {
				return _allocator.allocate(size);
			}
			return _fallback.allocate(size);
		}

		void deallocate(void* ptr, usize size) noexcept {
			if(align_up_to_max(size) <= aligned_size) {
				_allocator.deallocate(ptr, size);
			} else {
				_fallback.deallocate(ptr, size);
			}
		}

	private:
		Allocator _allocator;
		Fallback _fallback;
};

template<usize BlockSize, typename Allocator>
class StackBlockAllocator : NonCopyable {
	public:
		StackBlockAllocator() = default;

		StackBlockAllocator(StackBlockAllocator&& other) {
			std::swap(_allocator, other._allocator);
			std::swap(_block, other._block);
			std::swap(_allocated, other._allocated);
		}

		StackBlockAllocator(Allocator&& a) : _allocator(std::move(a)) {
		}


		~StackBlockAllocator() {
			if(_block) {
				_allocator.deallocate(_block, BlockSize);
			}
		}

		[[nodiscard]] void* allocate(usize size) noexcept {
			size = align_up_to_max(size);
			if(size + _allocated > BlockSize) {
				return nullptr;
			}

			u8* end = _block + _allocated;
			_allocated += size;
			return end;
		}

		void deallocate(void* ptr, usize size) noexcept {
			size = align_up_to_max(size);
			_allocated -= size;
			if(_block + _allocated != ptr) {
				y_fatal("Invalid deallocation.");
			}
		}

	private:
		Allocator _allocator;
		u8* _block = reinterpret_cast<u8*>(_allocator.allocate(BlockSize));
		usize _allocated = 0;
};

template<usize Size, typename Allocator, typename Fallback = NullAllocator>
class FixedSizeFreeListAllocator : NonCopyable {
	struct Node {
		Node* next = nullptr;
	};

	public:
		static constexpr usize aligned_size = align_up_to_max(std::min(sizeof(Node), Size));

		FixedSizeFreeListAllocator() = default;

		FixedSizeFreeListAllocator(FixedSizeFreeListAllocator&& other) {
			std::swap(_allocator, other._allocator);
			std::swap(_fallback, other._fallback);
			std::swap(_head, other._head);
		}

		FixedSizeFreeListAllocator(Allocator&& a) : _allocator(std::move(a)) {
		}

		FixedSizeFreeListAllocator(Allocator&& a, Fallback&& f) : _allocator(std::move(a)), _fallback(std::move(f)) {
		}

		~FixedSizeFreeListAllocator() {
			while(_head) {
				Node* next = _head->next;
				_allocator.deallocate(_head, aligned_size);
				_head = next;
			}
		}

		[[nodiscard]] void* allocate(usize size) noexcept {
			if(align_up_to_max(size) > aligned_size) {
				return _fallback.allocate(size);
			}
			if(_head) {
				void* ptr = _head;
				_head = _head->next;
				return ptr;
			}
			return _allocator.allocate(size);
		}

		void deallocate(void* ptr, usize size) noexcept {
			if(align_up_to_max(size) <= aligned_size) {
				Node* node = reinterpret_cast<Node*>(ptr);
				node->next = _head;
				_head = node;
			} else {
				_fallback.deallocate(ptr, size);
			}
		}


	private:
		Allocator _allocator;
		Fallback _fallback;
		Node* _head = nullptr;
};

template<typename Allocator>
class ElectricFenceAllocator : NonCopyable {
	public:
		static constexpr usize aligned_size = fixed_allocator_size_v<Allocator>;

		static constexpr usize fence_size = align_up_to_max(1024);
		static constexpr u8 fence = 0xFE;

		ElectricFenceAllocator() = default;
		ElectricFenceAllocator(ElectricFenceAllocator&&) = default;

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
			u8* f_end = static_cast<u8*>(ptr);
			u8* f_begin = f_end - fence_size;
			if(ptr) {
				u8* s_begin = f_end + size;
				u8* s_end = s_begin + fence_size;
				auto is_fence = [](u8 c) { return c == fence; };
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
		static constexpr usize aligned_size = fixed_allocator_size_v<Allocator>;

		LeakDetectorAllocator() = default;

		LeakDetectorAllocator(LeakDetectorAllocator&& other) {
			std::swap(_allocator, other._allocator);
			std::swap(_alive, other._alive);
		}

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
