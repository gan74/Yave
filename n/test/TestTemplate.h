/*******************************
Copyright (C) 2013-2014 grégoire ANGERAND

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

#ifndef N_TEST_TESTTEMPLATE_H
#define N_TEST_TESTTEMPLATE_H

#include <n/defines.h>
#include <n/utils.h>
#include "Test.h"

namespace n {
namespace test {

template<typename T>
class TestTemplate
{
	class TestRunner
	{
		public:
			TestRunner() {
				test(T().run(), true, "Auto tests failed");
			}
	};


	public:
		TestTemplate() {
			#ifdef N_AUTO_TEST
			n::unused(runner);
			#endif
		}

		virtual bool run() = 0;


	private:
	#ifdef N_AUTO_TEST
	static TestRunner runner;
	#endif
};

#ifdef N_AUTO_TEST
template<typename T>
typename TestTemplate<T>::TestRunner TestTemplate<T>::runner = TestTemplate<T>::TestRunner();
#endif

} //test
} //n

#endif // TESTTEMPLATE_H
