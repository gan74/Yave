/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/
#ifndef Y_TEST_H
#define Y_TEST_H

#include <y/utils.h>

namespace y {
namespace test {
namespace detail {

struct TestResult {
	bool result;
	const char* file;
	int line;
};

const char* test_box_msg(const char* msg = nullptr);
void test_assert(const char* msg, void (*func)(TestResult &));

}
}
}

#define Y_TEST_LINE_HELPER(prefix, LINE) _test_ ## prefix ## _at_ ## LINE
#define Y_TEST_HELPER(prefix, LINE) Y_TEST_LINE_HELPER(prefix, LINE)
#define Y_TEST_FUNC Y_TEST_HELPER(func, __LINE__)
#define Y_TEST_RUNNER Y_TEST_HELPER(runner, __LINE__)
#define Y_TEST_FAILED y::test::detail::TestResult { false, __FILE__, __LINE__ }

#define y_test_assert(t) do { if(!(t)) { _test_result = Y_TEST_FAILED; return; } } while(0)

#ifdef Y_AUTO_TEST

#define y_test_func(msg)																				\
static void Y_TEST_FUNC(y::test::detail::TestResult &);													\
namespace {																								\
	class Y_TEST_RUNNER {																				\
		Y_TEST_RUNNER() {																				\
			y::test::detail::test_assert(y::test::detail::test_box_msg(msg), &Y_TEST_FUNC);				\
		}																								\
		static Y_TEST_RUNNER runner;																	\
	};																									\
	Y_TEST_RUNNER Y_TEST_RUNNER::runner = Y_TEST_RUNNER();												\
}																										\
void Y_TEST_FUNC(y::test::detail::TestResult& _test_result)

#else

#define y_test_func(msg)																				\
static void Y_TEST_FUNC(y::test::detail::TestResult &);													\
namespace {																								\
	class Y_TEST_RUNNER {																				\
		Y_TEST_RUNNER() {																				\
			unused(Y_TEST_FUNC);																		\
		}																								\
	};																									\
}																										\
void Y_TEST_FUNC(y::test::detail::TestResult& _test_result)

#endif

#endif // Y_TEST_H
