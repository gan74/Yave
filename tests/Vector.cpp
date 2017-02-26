/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#include <y/core/Vector.h>
#include <y/core/SmallVector.h>
#include <y/test/test.h>
#include <vector>

//#include <y/core/String.h>

using namespace y;
using namespace y::core;

struct Big {
	u64 _big[256];
};

struct Base {
};
struct Derived : Base {
};
struct MoreDerived : Derived {
};

struct Polymorphic {
	virtual ~Polymorphic() {
	}
};

struct RaiiCounter : NonCopyable {
	RaiiCounter(usize* ptr) : counter(ptr) {
	}

	RaiiCounter(RaiiCounter&& raii) : counter(nullptr) {
		std::swap(raii.counter, counter);
	}

	~RaiiCounter() {
		if(counter) {
			(*counter)++;
		}
	}

	usize* counter;
};

template<typename T>
struct FakeAllocator {
	using value_type = T;
	using propagate_on_container_move_assignment = std::false_type;

	template<typename... Args>
	T* allocate(Args&&...) {
		return fatal("SmallVector allocated");
	}

	template<typename... Args>
	void deallocate(Args&&...) {
		fatal("SmallVector deallocated");
	}
};


template<typename T, usize Size = 8>
using SmallVec = SmallVector<T, Size, DefaultVectorResizePolicy, FakeAllocator<T>>;

static_assert(std::is_same<std::common_type<MoreDerived, Derived>::type, Derived>::value, "std::common_type failure");
static_assert(std::is_polymorphic<Polymorphic>::value, "std::is_polymorphic failure");
static_assert(!std::is_polymorphic<Polymorphic*>::value, "std::is_polymorphic failure");
static_assert(sizeof(Vector<int>) == 3 * sizeof(int*), "sizeof(Vector) is not 3 * sizeof(void*)");

/*template<typename P = DefaultVectorResizePolicy>
static void cout_vec_sizes(usize max, P p = P()) {
	for(usize i = 0, last = 0; i != max; ++i) {
		usize c = p.ideal_capacity(i);
		if(c != last) {
			std::cout << c << " ";
			last = c;
		}
	}
	std::cout << std::endl;
}*/

y_test_func("DefaultVectorResizePolicy") {
	DefaultVectorResizePolicy size;

	y_test_assert(size.ideal_capacity(0) == 0);
	//y_test_assert(size.shrink(0, 1));

	for(usize i = 0; i != size.threshold + 3 * size.step; i++) {
		y_test_assert(size.ideal_capacity(i) >= i);
	}
	//cout_vec_sizes(1024 * 32);
}

y_test_func("Vector creation") {
	Vector<int> vec;
	y_test_assert(vec.size() == 0);
	y_test_assert(vec.capacity() == 0);
}

y_test_func("Vector operator=") {
	Vector<int> vec = {1, 2, 3, 4};
	y_test_assert(vec == vector({1, 2, 3, 4}));

	vec = {};
	y_test_assert(vec.size() == 0);

	vec = {1, 2, 3};
	vec.push_back({4, 5});
	y_test_assert(vec == vector({1, 2, 3, 4, 5}));
}

y_test_func("Vector push_back") {
	Vector<int> vec;

	vec.set_capacity(19);
	y_test_assert(vec.capacity() >= 19);

	vec.push_back(0);
	vec.push_back(1);
	y_test_assert(vec.size() == 2);

	for(int i = 0; i != 18; i++) {
		vec.push_back(i);
	}
	vec.push_back() = 18;

	y_test_assert(vec.size() == 21);
	y_test_assert(vec.capacity() >= 21);
}

/*y_test_func("Vector erase") {
	Vector<int> vec;

	for(int i = 0; i != 10; i++) {
		vec.push_back(i);
	}
	y_test_assert(*vec.erase(vec.begin()) == 1);
	y_test_assert(vec == vector({1, 2, 3, 4, 5, 6, 7, 8, 9}));

	y_test_assert(*vec.erase(vec.begin() + 3) == 5);
	y_test_assert(vec == vector({1, 2, 3, 5, 6, 7, 8, 9}));

	y_test_assert(vec.erase(vec.end() - 1) == vec.end());
	y_test_assert(vec == vector({1, 2, 3, 5, 6, 7, 8}));
}*/

y_test_func("Vector erase_unordered") {
	Vector<int> vec;

	for(int i = 0; i != 10; i++) {
		vec.push_back(i);
	}
	vec.erase_unordered(vec.begin());
	y_test_assert(vec == vector({9, 1, 2, 3, 4, 5, 6, 7, 8}));

	vec.erase_unordered(vec.begin() + 4);
	y_test_assert(vec == vector({9, 1, 2, 3, 8, 5, 6, 7}));

	vec.erase_unordered(vec.end() - 1);
	y_test_assert(vec == vector({9, 1, 2, 3, 8, 5, 6}));
}

y_test_func("Vector shrink") {
	const usize max = 256;

	Vector<int> vec;

	while(vec.size() != max) {
		vec.push_back(0);
	}

	y_test_assert(vec.size() == max);
	y_test_assert(vec.capacity() >= max);

	vec.set_capacity(max / 2);
	y_test_assert(vec.size() == vec.capacity());
	y_test_assert(vec.capacity() == max / 2);
}

y_test_func("Vector clear") {
	const usize max = 256;

	Vector<int> vec;

	while(vec.size() != max) {
		vec.push_back(0);
	}
	vec.clear();
	y_test_assert(vec.size() == 0);
	y_test_assert(vec.capacity() == 0);

	while(vec.size() != max * 2) {
		vec.push_back(0);
	}
	vec.clear();
	y_test_assert(vec.size() == 0);
	y_test_assert(vec.capacity() == 0);
}

y_test_func("Vector iteration") {
	const int max = 256;

	Vector<int> vec;

	for(int i = 0; i != max; i++) {
		vec.push_back(i);
	}

	int counter = 0;
	for(int i : vec) {
		y_test_assert(i == counter++);
	}
}

y_test_func("Vector vector(...)") {
	auto vec = vector({1, 2, 3, 4, 5, 6, 7, 8});
	y_test_assert(vec.capacity() >= 8);
	y_test_assert(vec.size() == 8);

	int counter = 0;
	for(int i : vec) {
		y_test_assert(i == ++counter);
	}
}

y_test_func("Vector dtors") {
	usize counter = 0;
	auto vec = Vector<RaiiCounter>();
	vec.push_back(std::move(RaiiCounter(&counter)));

	y_test_assert(counter == 0);

	auto cap = vec.capacity();
	do {
		vec.push_back(RaiiCounter(&counter));
	} while(cap == vec.capacity() || vec.capacity() < 32);

	y_test_assert(counter == 0);

	usize total = vec.size();
	vec = Vector<RaiiCounter>();

	y_test_assert(counter == total);
}

y_test_func("SmallVector allocation") {
	SmallVec<int, 4> vec = vector({1, 2, 3, 4});
	y_test_assert(vec.capacity() == 4);
	y_test_assert(vec == vector({1, 2, 3, 4}));
}

y_test_func("SmallVector size") {
	SmallVector<Big> vec({Big{}});
	y_test_assert(vec.size() == 1);
}
