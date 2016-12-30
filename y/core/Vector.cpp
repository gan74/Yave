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
#include "Vector.h"
#include <y/test/test.h>
#include <vector>

namespace y {
namespace core {

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


static_assert(std::is_same<std::common_type<MoreDerived, Derived>::type, Derived>::value, "std::common_type failure");
static_assert(std::is_polymorphic<Polymorphic>::value, "std::is_polymorphic failure");
static_assert(!std::is_polymorphic<Polymorphic*>::value, "std::is_polymorphic failure");

y_test_func("DefaultVectorResizePolicy") {
	DefaultVectorResizePolicy size;

	y_test_assert(size.ideal_capacity(0) == 0);
	y_test_assert(size.shrink(0, 1));

	for(usize i = 0; i != size.threshold + 3 * size.step; i++) {
		y_test_assert(size.ideal_capacity(i) >= i);
	}
}


y_test_func("Vector creation") {
	Vector<int> vec;
	y_test_assert(vec.size() == 0);
	y_test_assert(vec.capacity() == 0);
}

y_test_func("Vector append") {
	Vector<int> vec;

	vec.set_capacity(19);
	y_test_assert(vec.capacity() >= 19);

	vec.append(0);
	vec.append(1);
	y_test_assert(vec.size() == 2);

	for(int i = 0; i != 18; i++) {
		vec.append(i);
	}

	y_test_assert(vec.size() == 20);
	y_test_assert(vec.capacity() >= 20);
}

y_test_func("Vector shrink") {
	const usize max = 256;

	Vector<int> vec;

	while(vec.size() != max) {
		vec.append(0);
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
		vec.append(0);
	}
	vec.clear();
	y_test_assert(vec.size() == 0);
	y_test_assert(vec.capacity() == 0);

	while(vec.size() != max * 2) {
		vec.append(0);
	}
	vec.clear();
	y_test_assert(vec.size() == 0);
	y_test_assert(vec.capacity() == 0);
}

y_test_func("Vector iteration") {
	const int max = 256;

	Vector<int> vec;

	for(int i = 0; i != max; i++) {
		vec.append(i);
	}

	int counter = 0;
	for(int i : vec) {
		y_test_assert(i == counter++);
	}
}

y_test_func("Vector vector(...)") {
	auto vec = vector(1, 2, 3, 4, 5, 6, 7, 8);
	y_test_assert(vec.capacity() >= 8);
	y_test_assert(vec.size() == 8);

	int counter = 0;
	for(int i : vec) {
		y_test_assert(i == ++counter);
	}
}

y_test_func("Vector<Range<I>>") {
	{
		auto r = range(1, 7);

		auto vec = vector(1, 2, 3, 4, 5, 6);
		vec.append(r);
		vec.append(r.reverse());
		vec.append(r.map([](int i) { return i + 1; }));
		vec.append(range(0, 10));

		y_test_assert(vec.size() == 6 * 4 + 10);
		y_test_assert(vec == vector({1, 2, 3, 4, 5, 6,
									 1, 2, 3, 4, 5, 6,
									 6, 5, 4, 3, 2, 1,
									 2, 3, 4, 5, 6, 7,
									 0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
	}
	{
		Vector<Range<ValueIterator<int>>> vec;
		vec.append(range(7, 11));

		y_test_assert(vec.size() == 1);
		y_test_assert(vec[0].collect<Vector>() == vector(7, 8, 9, 10));
	}
	{
		std::vector<int> std_vec;
		std_vec.push_back(1);
		std_vec.push_back(2);
		std_vec.push_back(3);

		Vector<int> vec = vector(0);
		vec.append(range(std_vec));

		y_test_assert(vec.size() == 4);
		y_test_assert(vec == vector(0, 1, 2, 3));
	}
}

y_test_func("Vector dtors") {
	usize counter = 0;
	auto vec = vector(std::move(RaiiCounter(&counter)));

	y_test_assert(counter == 0);

	auto cap = vec.capacity();
	do {
		vec.append(RaiiCounter(&counter));
	} while(cap == vec.capacity() || vec.capacity() < 32);

	y_test_assert(counter == 0);

	usize total = vec.size();
	vec = vector<RaiiCounter>();

	y_test_assert(counter == total);
}



}
}
