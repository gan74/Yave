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

namespace y {
namespace test {

const char *test_box_msg(const char *msg = 0);
void test_assert(const char *msg, bool sucess, const char *file, int line);

}
}

#ifdef Y_AUTO_TEST
#define TEST_ASSERT(func, msg) y::test::test_assert(msg, func, __FILE__, __LINE__)
#else
#define TEST_ASSERT(func, msg)
#endif

#define N_TEST_LINE_HELPER(prefix, LINE) _test_ ## prefix ## _at_ ## LINE
#define N_TEST_HELPER(prefix, LINE) N_TEST_LINE_HELPER(prefix, LINE)

#define N_TEST_FUNC N_TEST_HELPER(func, __LINE__)
#define N_TEST_RUNNER N_TEST_HELPER(runner, __LINE__)

#define test_func(msg)																					\
extern bool N_TEST_FUNC();																				\
namespace test {																						\
	class N_TEST_RUNNER {																				\
		N_TEST_RUNNER() {																				\
			TEST_ASSERT(N_TEST_FUNC(), msg);															\
		}																								\
		static N_TEST_RUNNER runner;																	\
	};																									\
	N_TEST_RUNNER N_TEST_RUNNER::runner = N_TEST_RUNNER();												\
}																										\
bool N_TEST_FUNC()

#endif // Y_TEST_H
