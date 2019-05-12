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
#include <y/core/SlotMap.h>
#include <y/test/test.h>
#include <vector>
#include <memory>

namespace {

using namespace y;
using namespace y::core;

y_test_func("SlotMap insert") {
	SlotMap<int> map;
	for(int i = 0; i != 100; ++i) {
		map.insert(i);
	}
	y_test_assert(map.size() == 100);
}

y_test_func("SlotMap insert - erase") {
	SlotMap<int> map;
	core::Vector<SlotMapKey<int>> keys;

	for(int i = 0; i != 100; ++i) {
		keys << map.insert(i);
	}
	y_test_assert(map.size() == 100);

	for(usize i = 0; i != 50; ++i) {
		map.erase(keys[i * 2]);
	}
	y_test_assert(map.size() == 50);

	for(usize i = 0; i != 50; ++i) {
		y_debug_assert(map.contains(keys[i * 2 + 1]));
	}

	int index = 1;
	for(int i : map) {
		y_debug_assert(i == index);
		index += 2;
	}
}

}
