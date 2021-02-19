/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#include <y/core/RingQueue.h>
#include <y/test/test.h>
#include <vector>
#include <memory>

//#include <y/core/String.h>

namespace {
using namespace y;
using namespace y::core;

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
            ++(*counter);
        }
    }

    usize* counter;
};

y_test_func("RingQueue creation") {
    const usize cap = 256;
    const RingQueue<int> q(cap);
    y_test_assert(q.size() == 0);
    y_test_assert(q.capacity() == cap);
}

y_test_func("RingQueue push/pop") {
    const usize cap = 256;
    RingQueue<usize> q(cap);

    for(usize i = 0; i != cap / 2; ++i) {
        q.push(i);
        y_test_assert(q.last() == i);
        y_test_assert(q.size() == i + 1);
    }
    for(usize i = 0; i != cap / 2; ++i) {
        y_test_assert(q.pop() == i);
    }
}

y_test_func("RingQueue full") {
    const usize cap = 256;
    RingQueue<usize> q(cap);
    y_test_assert(q.is_empty());

    for(usize i = 0; i != cap; ++i) {
        q.push(i);
    }

    y_test_assert(!q.is_empty());
    y_test_assert(q.is_full());
    y_test_assert(q.size() == cap);

    for(usize i = 0; i != cap; ++i) {
        y_test_assert(q.pop() == i);
    }
}

}

