/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include <y/core/AssocVector.h>
#include <y/core/String.h>
#include <y/test/test.h>

namespace {
using namespace y;
using namespace y::core;

static_assert(std::is_same_v<usize, std::remove_reference<decltype(std::declval<AssocVector<usize, usize>>()[usize(1)])>::type>, "AssocVector::operator[] returns the wrong type");

struct NonCopyableValue : NonCopyable {
    NonCopyableValue() : value(-1) {
    }

    NonCopyableValue(int v) : value(v) {
    }

    NonCopyableValue(NonCopyableValue&& other) : value(other.value) {
        other.value = -2;
    }

    ~NonCopyableValue() {
    }

    NonCopyableValue &operator=(NonCopyableValue&& other) {
        value = std::exchange(other.value, -2);
        return *this;
    }

    int value;
};

y_test_func("AssocVector creation") {
    AssocVector<usize, usize> av;
    for(usize i = 0; i != 16; ++i) {
        y_test_assert(av[i] == 0);
        av[i] = i + 1;
    }
    for(auto i : av) {
        y_test_assert(i.first == i.second - 1);
    }
}

y_test_func("AssocVector movable values") {
    AssocVector<usize, NonCopyableValue> av;
    for(usize i = 0; i != 16; ++i) {
        av[i] = NonCopyableValue(int(i));
    }
    for(usize i = 0; i != av.size(); ++i) {
        const auto& value = av[i];
        y_test_assert(value.value == int(i));
    }
}
}

