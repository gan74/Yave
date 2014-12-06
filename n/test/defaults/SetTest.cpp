/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

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

#include <n/core/Set.h>
#include <n/core/Array.h>
#include <n/test/TestTemplate.h>
#include <n/test/Test.h>

namespace n {
namespace test {

class SetTest : public TestTemplate<SetTest>
{
	public:
		SetTest() : TestTemplate<SetTest>() {
		}

		bool run() override {
			basicTests<int>();
			return true;
		}

	private:
		template<typename T>
		void basicTests() {
			core::Set<T> s;
			T max = 10;//core::random(1000, 10000);
			core::Array<T> testArray = genTestData<T>(max);
			for(T i : testArray) {
				s.insert(i);
				if(i % 7 == 0) {
					s.insert(i);
				}
			}
			T i = 0;
			for(T e : s) {
				test(e, i, "Ordering test failed");
				i++;
			}
			for(T w = 0; w < max; w += (w % 3) ? 5 : 13) {
				auto it = s.find(w);
				if(test(it == s.end(), false, "Find test failed")) {
					test(*it, w, "Find result test failed");
				}
			}
			auto s2 = s.filtered([](T a) -> bool { return a % 2; });
			s.filter([](T a) -> bool { return a % 2; });
			for(T w = 1; w < max; w += 2) {
				auto it = s.find(w);
				auto it2 = s.find(w - 1);
				if(test(it != s.end() && it2 == s.end(), true, "Filter test failed")) {
					test(*it, w, "Filter result test failed");
				}
			}
			for(T w = 1; w < max; w += 2) {
				auto it = s2.find(w);
				auto it2 = s2.find(w - 1);
				if(test(it == s2.end(), false, "Filtered pass test failed")) {
					test(it2 == s2.end(), true, "Filtered remove test failed");
					test(*it, w, "Filtered result test failed");
				}
			}
			test(s, s2, "Equality test failed");
		}

		template<typename T>
		core::Array<T> genTestData(int max) {
			core::Array<T> testArray(max);
			for(int i = 0; i != max; i++) {
				testArray.append(i);
			}
			testArray.shuffle();
			return testArray;
		}
};

} // test
} //n



