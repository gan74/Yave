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
#include <y/core/Vector.h>
#include <y/test/test.h>

#include <y/utils/traits.h>

#include <vector>
#include <memory>

//#include <y/core/String.h>

namespace {
using namespace y;
using namespace y::core;

static_assert(is_iterable<const core::Vector<int>&>);
static_assert(is_iterable<core::Vector<int>&>);

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
            ++(*counter);
        }
    }

    usize* counter;
};

struct LogCopyMove {
    LogCopyMove(LogCopyMove&& other) : copies(other.copies), moves(other.moves) {
        ++(*moves);
    }

    LogCopyMove(const LogCopyMove& other) : copies(other.copies), moves(other.moves) {
        ++(*copies);
    }

    LogCopyMove& operator=(LogCopyMove&& other) {
        unused(other);
        y_debug_assert(copies == other.copies && moves == other.moves);
        ++(*moves);
        return *this;
    }

    LogCopyMove& operator=(const LogCopyMove& other) {
        unused(other);
        y_debug_assert(copies == other.copies && moves == other.moves);
        ++(*copies);
        return *this;
    }

    LogCopyMove(usize* c, usize* m) : copies(c), moves(m) {
    }

    usize* copies = nullptr;
    usize* moves = nullptr;
};

template<typename T>
struct FakeAllocator {
    using value_type = T;
    using size_type = usize;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::false_type;

    template<typename... Args>
    T* allocate(Args&&...) {
        /*return*/ y_fatal("SmallVector allocated");
    }

    template<typename... Args>
    void deallocate(Args&&...) {
        y_fatal("SmallVector deallocated");
    }
};


template<typename T, usize Size = 8>
using SmallVec = SmallVector<T, Size, FakeAllocator<T>>;

static_assert(std::is_same_v<std::common_type<MoreDerived, Derived>::type, Derived>, "std::common_type failure");
static_assert(std::is_polymorphic_v<Polymorphic>, "std::is_polymorphic failure");
static_assert(!std::is_polymorphic_v<Polymorphic*>, "std::is_polymorphic failure");

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
    const DefaultVectorResizePolicy size;

    y_test_assert(size.ideal_capacity(0) == 0);
    //y_test_assert(size.shrink(0, 1));

    for(usize i = 0; i != 1 << 16; ++i) {
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
    y_test_assert(vec == Vector({1, 2, 3, 4}));

    vec = {};
    y_test_assert(vec.size() == 0);

    vec = {1, 2, 3};
    vec << 4 << 5;
    y_test_assert(vec == Vector({1, 2, 3, 4, 5}));
}

y_test_func("Vector push_back") {
    Vector<int> vec;

    vec.set_capacity(19);
    y_test_assert(vec.capacity() >= 19);

    vec.push_back(0);
    vec.push_back(1);
    y_test_assert(vec.size() == 2);

    for(int i = 0; i != 18; ++i) {
        vec.push_back(i);
    }
    vec.emplace_back() = 18;

    y_test_assert(vec.size() == 21);
    y_test_assert(vec.capacity() >= 21);
}

/*y_test_func("Vector erase") {
    Vector<int> vec;

    for(int i = 0; i != 10; ++i) {
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

    for(int i = 0; i != 10; ++i) {
        vec.push_back(i);
    }
    vec.erase_unordered(vec.begin());
    y_test_assert(vec == Vector({9, 1, 2, 3, 4, 5, 6, 7, 8}));

    vec.erase_unordered(vec.begin() + 4);
    y_test_assert(vec == Vector({9, 1, 2, 3, 8, 5, 6, 7}));

    vec.erase_unordered(vec.end() - 1);
    y_test_assert(vec == Vector({9, 1, 2, 3, 8, 5, 6}));
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

    for(int i = 0; i != max; ++i) {
        vec.push_back(i);
    }

    int counter = 0;
    for(int i : vec) {
        y_test_assert(i == counter++);
    }
}

y_test_func("Vector vector(...)") {
    const auto vec = Vector({1, 2, 3, 4, 5, 6, 7, 8});
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
    vec.push_back(RaiiCounter(&counter));

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
    SmallVec<int, 4> vec = {1, 2, 3, 4};
    y_test_assert(vec.capacity() == 4);
    y_test_assert(vec == Vector({1, 2, 3, 4}));
}

y_test_func("SmallVector size") {
    const SmallVector<Big> vec({Big{}});
    y_test_assert(vec.size() == 1);
}

y_test_func("SmallVector copy") {
    {
        const SmallVector<int, 8> vec = {1, 2, 3, 4};
        const SmallVector<int, 8> cpy(vec);
        y_test_assert(cpy.size() == 4);
        y_test_assert(cpy[0] == 1);
        y_test_assert(cpy[1] == 2);
        y_test_assert(cpy[2] == 3);
        y_test_assert(cpy[3] == 4);
    }
    {
        const std::shared_ptr<int> rc(new int(2));
        y_test_assert(rc.use_count() == 1);
        SmallVector<std::shared_ptr<int>, 8> vec;
        vec.push_back(rc);
        y_test_assert(rc.use_count() == 2);
        {
            const SmallVector<std::shared_ptr<int>, 8> cpy(vec);
            y_test_assert(rc.use_count() == 3);
        }
        y_test_assert(rc.use_count() == 2);
    }
}

y_test_func("SmallVector move") {
    {
        const SmallVector<int, 8> vec = {1, 2, 3, 4};
        const SmallVector<int, 8> cpy(std::move(vec));
        y_test_assert(cpy.size() == 4);
        y_test_assert(cpy[0] == 1);
        y_test_assert(cpy[1] == 2);
        y_test_assert(cpy[2] == 3);
        y_test_assert(cpy[3] == 4);
    }
    {
        const std::shared_ptr<int> rc(new int(7));
        SmallVector<std::shared_ptr<int>, 8> vec;
        vec.push_back(rc);
        y_test_assert(rc.use_count() == 2);
        {
            y_test_assert(rc.use_count() == 2);
            const SmallVector<std::shared_ptr<int>, 8> cpy(std::move(vec));
            y_test_assert(rc.use_count() == 2);
        }
        y_test_assert(rc.use_count() == 1);
    }
}

y_test_func("Vector insert") {
    core::Vector<int> v;
    v.push_back(0);
    v.push_back(1);
    v.push_back(2);
    v.push_back(4);
    v.push_back(5);
    v.push_back(6);

    const auto it = std::find(v.begin(), v.end(), 4);
    y_test_assert(*it == 4);
    v.insert(it, 3);

    for(int i = 0; i != int(v.size()); ++i) {
        y_test_assert(v[i] == i);
    }
}

y_test_func("Vector copy-swap") {
    usize copies = 0;
    usize moves = 0;
    core::Vector<LogCopyMove> v;
    v.emplace_back(&copies, &moves);
    v.emplace_back(&copies, &moves);
    v.emplace_back(&copies, &moves);
    v.emplace_back(&copies, &moves);
    v.emplace_back(&copies, &moves);

    y_test_assert(copies == 0);
    y_test_assert(moves == 0);

    core::Vector<LogCopyMove> s;
    std::swap(v, s);

    y_test_assert(copies == 0);
    y_test_assert(moves == 0);
}


y_test_func("SmallVector copy-swap") {
    usize copies = 0;
    usize moves = 0;
    core::SmallVector<LogCopyMove, 4> v;
    v.emplace_back(&copies, &moves);
    v.emplace_back(&copies, &moves);
    v.emplace_back(&copies, &moves);
    v.emplace_back(&copies, &moves);

    y_test_assert(copies == 0);
    y_test_assert(moves == 0);

    core::SmallVector<LogCopyMove, 4> s;
    std::swap(v, s);

    y_test_assert(copies == 0);
    y_test_assert(moves == 8); // swap = 2 moves
}

}

