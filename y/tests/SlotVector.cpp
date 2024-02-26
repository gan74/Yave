/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include <y/core/SlotVector.h>
#include <y/core/String.h>
#include <y/test/test.h>

namespace {
using namespace y;
using namespace y::core;

y_test_func("SlotVector creation") {
    SlotVector<usize> vec;

    for(usize i = 0; i != 123; ++i) {
        y_test_assert(vec[vec.insert(i)] == i);
    }

    y_test_assert(vec.size() == 123);
}

y_test_func("SlotVector clear") {
    SlotVector<usize> vec;

    for(usize i = 0; i != 123; ++i) {
        y_test_assert(vec[vec.insert(i)] == i);
    }

    vec.clear();
    y_test_assert(vec.is_empty());
    y_test_assert(vec.size() == 0);
}

y_test_func("SlotVector erase") {
    SlotVector<usize> vec;

    for(usize i = 0; i != 123; ++i) {
        y_test_assert(vec[vec.insert(i)] == i);
    }

    const auto it = std::find(vec.begin(), vec.end(), 37);
    y_test_assert(it != vec.end());
    y_test_assert(*it == 37);

    vec.erase(it);

    y_test_assert(std::find(vec.begin(), vec.end(), 37) == vec.end());
}

y_test_func("SlotVector sort") {
    SlotVector<int> vec;

    for(int i = 0; i != 1024; ++i) {
        vec.insert(i % 17);
    }

    auto compare = [](int a, int b) {
        return b < a;
    };

    vec.sort(compare);

    y_test_assert(std::is_sorted(vec.begin(), vec.end(), compare));
}

}

