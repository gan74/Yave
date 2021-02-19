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
#ifndef Y_TEST_TEST_H
#define Y_TEST_TEST_H

#include <y/utils.h>

namespace y {
namespace test {

namespace detail {
struct TestResult {
    bool result;
    const char* file;
    int line;
};

struct TestItem {
    const char* name = "Unknown test";
    void (*test_func)(TestResult&) = nullptr;
    TestItem* next = nullptr;
};

void register_test(TestItem* test);
}

bool run_tests();

}
}

#ifdef Y_BUILD_TESTS

#define Y_TEST_FUNC y_create_name_with_prefix(func)
#define Y_TEST_RUNNER y_create_name_with_prefix(runner)
#define Y_TEST_FAILED y::test::detail::TestResult { false, __FILE__, __LINE__ }

#define y_test_assert(t) do { if(!(t)) { _y_test_result = Y_TEST_FAILED; return; } } while(false)

#define y_test_func(name)                                                                               \
static void Y_TEST_FUNC(y::test::detail::TestResult&);                                                  \
namespace {                                                                                             \
    class Y_TEST_RUNNER {                                                                               \
        Y_TEST_RUNNER() : test_item({name, &Y_TEST_FUNC, nullptr}) {                                    \
            y::test::detail::register_test(&test_item);                                                 \
        }                                                                                               \
        y::test::detail::TestItem test_item;                                                            \
        static Y_TEST_RUNNER runner;                                                                    \
    };                                                                                                  \
    Y_TEST_RUNNER Y_TEST_RUNNER::runner = Y_TEST_RUNNER();                                              \
}                                                                                                       \
void Y_TEST_FUNC(y::test::detail::TestResult& _y_test_result)

#else

#define y_test_assert(t) do { y::unused(t) } while(false)

#define y_test_func(name)                                                                               \
static void Y_TEST_FUNC(y::test::detail::TestResult& _y_test_result)

#endif

#endif // Y_TEST_TEST_H

