/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

#include <iterator>

namespace {
using namespace y;
using namespace y::core;

static_assert(std::bidirectional_iterator<PagedSet<int>::iterator>);
static_assert(std::bidirectional_iterator<PagedSet<int>::const_iterator>);
struct Counted {
    static inline isize alive = 0;
    static inline usize destroyed = 0;

    usize value = 0;

    Counted(usize v) : value(v) {
        ++alive;
    }

    ~Counted() {
        --alive;
        ++destroyed;
    }

    Counted(const Counted&) = delete;
    Counted& operator=(const Counted&) = delete;
};

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

y_test_func("PagedSet erase across pages") {
    PagedSet<usize> set;

    const usize count = set.page_size * 2 + 16;
    for(usize i = 0; i != count; ++i) {
        set.emplace(i);
    }

    for(usize i = 1; i < count; i += 2) {
        const auto it = std::find(set.begin(), set.end(), i);
        y_test_assert(it != set.end());
        set.erase(it);
    }

    y_test_assert(set.size() == count / 2);

    Vector<usize> values;
    for(usize v : set) {
        values.emplace_back(v);
    }
    std::sort(values.begin(), values.end());
    for(usize i = 0; i != values.size(); ++i) {
        y_test_assert(values[i] == i * 2);
    }
}

y_test_func("PagedSet references are stable and slots are reused") {
    PagedSet<usize> set;

    const usize* first = &set.emplace(usize(0));
    for(usize i = 1; i != set.page_size * 3; ++i) {
        set.emplace(i);
    }
    y_test_assert(*first == 0);

    const auto it = std::find(set.begin(), set.end(), 17);
    const usize* addr = &*it;
    set.erase(it);

    const usize* reused = &set.emplace(usize(1717));
    y_test_assert(reused == addr);
    y_test_assert(*first == 0);
    y_test_assert(set.size() == set.page_size * 3);
}

y_test_func("PagedSet sort_indices") {
    PagedSet<usize> set;

    for(usize i = 0; i != set.page_size + 64; ++i) {
        set.emplace(i);
    }

    for(usize i = 0; i < set.page_size + 64; i += 3) {
        set.erase(std::find(set.begin(), set.end(), i));
    }

    set.sort([](usize a, usize b) { return a > b; });
    set.sort_indices();

    // Value i was emplaced in slot i, so storage order matches value order
    y_test_assert(std::is_sorted(set.begin(), set.end()));

    // Free slots are sorted too: the next emplace reuses the lowest one (slot 0)
    const usize* lowest = &set.emplace(usize(0));
    y_test_assert(lowest + 1 == &*std::find(set.begin(), set.end(), 1));
}

y_test_func("PagedSet move and swap") {
    PagedSet<usize> a;
    PagedSet<usize> b;

    for(usize i = 0; i != 600; ++i) {
        a.emplace(i);
    }
    b.emplace(usize(42));

    const usize* addr = &*a.begin();

    a.swap(b);
    y_test_assert(a.size() == 1 && *a.begin() == 42);
    y_test_assert(b.size() == 600 && &*b.begin() == addr);

    PagedSet<usize> c(std::move(b));
    y_test_assert(b.is_empty());
    y_test_assert(c.size() == 600 && &*c.begin() == addr);

    a = std::move(c);
    y_test_assert(a.size() == 600 && &*a.begin() == addr);

    usize sum = 0;
    for(usize v : a) {
        sum += v;
    }
    y_test_assert(sum == 599 * 600 / 2);
}

y_test_func("PagedSet destroys elements exactly once") {
    Counted::alive = 0;
    Counted::destroyed = 0;

    {
        PagedSet<Counted, 16> set;
        for(usize i = 0; i != 40; ++i) {
            set.emplace(i);
        }
        y_test_assert(Counted::alive == 40);

        for(usize i = 0; i != 10; ++i) {
            set.erase(set.begin());
        }
        y_test_assert(Counted::alive == 30);
        y_test_assert(Counted::destroyed == 10);

        set.make_empty();
        y_test_assert(Counted::alive == 0);
        y_test_assert(Counted::destroyed == 40);

        for(usize i = 0; i != 20; ++i) {
            set.emplace(i);
        }
        y_test_assert(Counted::alive == 20);
    }

    y_test_assert(Counted::alive == 0);
    y_test_assert(Counted::destroyed == 60);
}

}

