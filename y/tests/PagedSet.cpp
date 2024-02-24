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

#include <y/core/PagedSet.h>
#include <y/core/String.h>
#include <y/test/test.h>

namespace {
using namespace y;
using namespace y::core;

y_test_func("PagedSet creation") {
    PagedSet<usize> set;

    for(usize i = 0; i != set.page_size + 16; ++i) {
        y_test_assert(set.emplace(i) == i);
    }

    y_test_assert(set.size() == set.page_size + 16);
}

y_test_func("PagedSet clear") {
    PagedSet<usize> set;

    for(usize i = 0; i != set.page_size + 16; ++i) {
        y_test_assert(set.emplace(i) == i);
    }

    set.clear();
    y_test_assert(set.is_empty());
    y_test_assert(set.size() == 0);
}

y_test_func("PagedSet erase") {
    PagedSet<usize> set;

    for(usize i = 0; i != 16; ++i) {
        y_test_assert(set.emplace(i) == i);
    }

    const auto it = std::find(set.begin(), set.end(), 7);
    y_test_assert(it != set.end());
    y_test_assert(*it == 7);

    set.erase(it);

    y_test_assert(std::find(set.begin(), set.end(), 7) == set.end());
}

y_test_func("PagedSet sort") {
    PagedSet<int> set;

    for(int i = 0; i != 1024; ++i) {
        set.emplace(i % 17);
    }

    auto compare = [](int a, int b) {
        return b < a;
    };

    set.sort(compare);

    y_test_assert(std::is_sorted(set.begin(), set.end(), compare));
}

}

