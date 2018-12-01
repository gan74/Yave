#include <y/test/test.h>
#include <y/mem/allocators.h>
#include <y/core/String.h>

using namespace y;
using namespace memory;

y_test_func("Test test") {
	y_test_assert(true);
}

y_test_func("StackBlockAllocator basics") {
	static constexpr usize size = align_up_to_max(1024);
	StackBlockAllocator<size, Mallocator> allocator;
	y_test_assert(allocator.allocate(size + 1) == nullptr);

	{
		void* p = allocator.allocate(size);
		y_test_assert(p);
		y_test_assert(allocator.allocate(4) == nullptr);
		allocator.deallocate(p, size);
	}

	{
		void* a = allocator.allocate(size / 2 - 2);
		void* b = allocator.allocate(size / 2);
		y_test_assert(a);
		y_test_assert(b);
		y_test_assert(allocator.allocate(1) == nullptr);
		allocator.deallocate(b, size / 2);
		allocator.deallocate(a, size / 2 - 1);
	}
}

y_test_func("FixedSizeFreeListAllocator basics") {
	static constexpr usize size = align_up_to_max(std::max(usize(8), max_alignment));
	static constexpr usize min_size = size - max_alignment + 1;
	//static_assert(align_up_to_max(size) == size);

	FixedSizeFreeListAllocator<size, Mallocator> allocator;

	y_test_assert(allocator.allocate(size + 1) == nullptr);
	void* p1 = allocator.allocate(size);
	void* p2 = allocator.allocate(min_size);
	y_test_assert(p1);
	y_test_assert(p2);
	y_test_assert(p1 != p2);

	allocator.deallocate(p1, size);
	void* p3 = allocator.allocate(size - 1);
	y_test_assert(p3 == p1);

	allocator.deallocate(p3, size - 1);
	allocator.deallocate(p2, min_size);
}


template<typename A>
struct Print : private A {
	static constexpr usize aligned_size = fixed_allocator_size_v<A>;

	core::String short_typename() const {
		auto name = type_name<A>();
		if(name.starts_with("y::")) {
			name = name.sub_str(3);
		}
		if(name.starts_with("")) {
			name = name.sub_str(8);
		}
		return name;
	}

	[[nodiscard]] void* allocate(usize size) noexcept {
		log_msg(fmt("% :: alloc(%)", short_typename(), size));
		return A::allocate(size);
	}

	void deallocate(void* ptr, usize size) noexcept {
		log_msg(fmt("% :: free(%)", short_typename(), size));
		A::deallocate(ptr, size);
	}
};


int main() {
	using Base = ElectricFenceAllocator<ElectricFenceAllocator<Print<LeakDetectorAllocator<Mallocator>>>>;
	using A = FixedSizeAllocator<32, Base>;
	using B = FixedSizeAllocator<16, Base>;
	using C = FixedSizeAllocator<64, Base>;
	using Alloc = BucketizerAllocator<Base, A, B, C>;
	{
		PolymorphicAllocator<Base> poly;
		PolymorphicAllocatorBase* ppoly = &poly;
		//C c(ppoly);
		auto alloc = Alloc();

		for(usize s = 1; s < 128; s += 3) {
			u8* ptr = static_cast<u8*>(alloc.allocate(s));

			if(!ptr) {
				y_fatal("Unable to allocate for size = %", s);
			}
			for(usize i = 0; i != s; ++i) {
				ptr[i] = 9;
			}

			alloc.deallocate(ptr, s);
		}
	}


	return 0;
}



