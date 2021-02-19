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

#include "test.h"

#include <y/utils.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

#include <cstring>

#include <iostream>

namespace y {
namespace test {
namespace detail {

static TestItem* first_test = nullptr;

void register_test(TestItem* test) {
    test->next = first_test;
    first_test = test;
}

static bool run_test(const detail::TestItem* test) {
    const char* ok      = "\x1b[32m  [ OK ]   \x1b[0m";
    const char* failure = "\x1b[31m[ FAILED ] \x1b[0m";

    y::detail::setup_console();

    std::cout << test->name << ":";
    for(usize size = std::strlen(test->name) + 1; size != 80 - 11; ++size) {
        std::cout << " ";
    }


    TestResult res{true, nullptr, 0};
    (test->test_func)(res);

    if(res.result) {
        std::cout << ok << std::endl;
    } else {
        std::cout << failure << std::endl;
        std::cerr << "\ty_test_assert failed: in file: " << res.file << " at line: "<< res.line << std::endl;;
    }
    return res.result;
}

}

bool run_tests() {
    bool all_ok = true;
    for(detail::TestItem* test = detail::first_test; test; test = test->next) {
        all_ok &= run_test(test);
    }
    return all_ok;
}

}
}

