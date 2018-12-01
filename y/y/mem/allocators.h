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

namespace y {
namespace memory {

namespace detail {

template<typename T>
static auto has_aligned_size(T*) -> bool_type<!std::is_void_v<decltype(T::aligned_size)>>;
template<typename T>
static auto has_aligned_size(...) -> std::false_type;

template<typename Allocator>
struct DerivedAllocator : private Allocator {
	[[nodiscard]] void* allocate(usize size) noexcept {
		return Allocator::allocate(size);
	}

	void deallocate(void* ptr, usize size) noexcept {
		Allocator::deallocate(ptr, size);
	}
};

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




class NullAllocator {
	public:
		[[nodiscard]] void* allocate(usize) noexcept {
			return nullptr;
		}

		void deallocate(void* ptr, usize) noexcept {
			y_debug_assert(!ptr);
		}
};

class Mallocator {
	public:
		[[nodiscard]] void* allocate(usize size) noexcept {
			return std::malloc(alloc::align_up(size));
		}

		void deallocate(void* ptr, usize size) noexcept {
			unused(size);
			std::free(ptr);
		}
};


template<usize Size, typename Allocator, typename Fallback = NullAllocator>
class FixedSizeAllocator : private Allocator, private Fallback {
	public:
		static constexpr usize aligned_size = alloc::align_up(Size);

		FixedSizeAllocator() {
		}

		[[nodiscard]] void* allocate(usize size) noexcept {
			if(alloc::align_up(size) == aligned_size) {
				return Allocator::allocate(size);
			}
			return Fallback::allocate(size);
		}

		void deallocate(void* ptr, usize size) noexcept {
			if(alloc::align_up(size) == aligned_size) {
				Allocator::deallocate(ptr, size);
			} else {
				Fallback::deallocate(ptr, size);
			}
		}
};

template<usize BlockSize, typename Allocator>
class StackBlockAllocator : private Allocator {
	public:
		StackBlockAllocator() : _block(reinterpret_cast<u8*>(Allocator::allocate(BlockSize))) {
		}

		~StackBlockAllocator() {
			Allocator::deallocate(_block, BlockSize);
		}

		[[nodiscard]] void* allocate(usize size) noexcept {
			size = alloc::align_up(size);
			if(size + _allocated > BlockSize) {
				return nullptr;
			}

			u8* end = _block + _allocated;
			_allocated += size;
			return end;
		}

		void deallocate(void* ptr, usize size) noexcept {
			size = alloc::align_up(size);
			_allocated -= size;
			if(_block + _allocated != ptr) {
				y_fatal("Invalid deallocation.");
			}
		}

	private:
		u8* _block = nullptr;
		usize _allocated = 0;
};

template<usize Size, typename Allocator, typename Fallback = NullAllocator>
class FixedSizeFreeListAllocator : private Allocator, private Fallback {
	struct Node {
		Node* next = nullptr;
	};

	public:
		static constexpr usize aligned_size = alloc::align_up(std::min(sizeof(Node), Size));

		FixedSizeFreeListAllocator() {
		}

		~FixedSizeFreeListAllocator() {
			while(_head) {
				Node* next = _head->next;
				Allocator::deallocate(_head, aligned_size);
				_head = next;
			}
		}

		[[nodiscard]] void* allocate(usize size) noexcept {
			if(alloc::align_up(size) != aligned_size) {
				return Fallback::allocate(size);
			}
			if(_head) {
				void* ptr = _head;
				_head = _head->next;
				return ptr;
			}
			return Allocator::allocate(size);
		}

		void deallocate(void* ptr, usize size) noexcept {
			if(alloc::align_up(size) == aligned_size) {
				Node* node = reinterpret_cast<Node*>(ptr);
				node->next = _head;
				_head = node;
			} else {
				Fallback::deallocate(ptr, size);
			}
		}


	private:
		Node* _head = nullptr;
};

template<typename Fallback, typename... Allocators>
class BucketizerAllocator : private Allocators..., private detail::DerivedAllocator<Fallback> {

	using FallbackAllocator = detail::DerivedAllocator<Fallback>;

	template<typename A, typename B>
	struct AllocatorsComparator {
		static constexpr bool value = fixed_allocator_size_v<A> > fixed_allocator_size_v<B>;
	};

	using sorted_allocators = sorted_tuple_t<std::tuple<Allocators...>, AllocatorsComparator>;

#define MAKE_DISPATCHER(name, func)													\
	template<usize N = 0, typename... Args>											\
	auto name(usize size, Args&&... args) {											\
		if constexpr(N < std::tuple_size_v<sorted_allocators>) {					\
			using Alloc = std::tuple_element_t<N, sorted_allocators>;				\
			if(size <= fixed_allocator_size_v<Alloc>) {								\
				return func<Alloc>(size, std::forward<Args>(args)...);				\
			}																		\
			return name<N + 1>(size, std::forward<Args>(args)...);					\
		} else {																	\
			return func<FallbackAllocator>(size, std::forward<Args>(args)...);		\
		}																			\
	}

	MAKE_DISPATCHER(dispatch_allocate, allocate_one)
	MAKE_DISPATCHER(dispatch_deallocate, deallocate_one)

#undef MAKE_DISPATCHER

	template<typename A>
	void* allocate_one(usize size) {
		return A::allocate(size);
	}

	template<typename A>
	void deallocate_one(usize size, void* ptr) {
		A::deallocate(ptr, size);
	}

	public:
		[[nodiscard]] void* allocate(usize size) noexcept {
			return this->dispatch_allocate(size);
		}

		void deallocate(void* ptr, usize size) noexcept {
			this->dispatch_deallocate(size, ptr);
		}
};


template<typename Allocator>
class ElectricFenceAllocator : private Allocator {
	public:
		static constexpr usize aligned_size = fixed_allocator_size_v<Allocator>;

		static constexpr usize fence_size = alloc::align_up(1024);
		static constexpr u8 fence = 0xFE;

		[[nodiscard]] void* allocate(usize size) noexcept {
			u8* f_begin = static_cast<u8*>(Allocator::allocate(2 * fence_size + size));
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
			Allocator::deallocate(f_begin, 2 * fence_size + size);
		}
};

template<typename Allocator>
class LeakDetectorAllocator : private Allocator {
	public:
		static constexpr usize aligned_size = fixed_allocator_size_v<Allocator>;

		~LeakDetectorAllocator() {
			if(_alive) {
				y_fatal("Memory was not freed before allocator destruction (% bytes leaked).", _alive);
			}
		}

		[[nodiscard]] void* allocate(usize size) noexcept {
			_alive += size;
			return Allocator::allocate(size);
		}

		void deallocate(void* ptr, usize size) noexcept {
			if(size > _alive) {
				y_fatal("More memory was freed than has been allocated (currently allocated: % bytes, trying to free % bytes).", _alive, size);
			}
			_alive -= size;
			Allocator::deallocate(ptr, size);
		}

	private:
		usize _alive = 0;
};

}
}


#endif // Y_MEM_ALLOCATORS_H
