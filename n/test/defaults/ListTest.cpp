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

#include <n/test/TestTemplate.h>
#include <n/core/List.h>
#include <n/core/Range.h>
#include <n/test/Test.h>

namespace n {
namespace test {

class ListTest : public TestTemplate<ListTest>
{
	public:
		ListTest() : TestTemplate<ListTest>() {
		}

		bool run() override {
			basicTests<int>();
			basicTests<Dummy>();
			return true;
		}

	private:
		struct Dummy
		{
			Dummy(int w = 0) : i(w) {
			}

			operator int() const {
				return i;
			}

			int i;
		};

		template<typename T>
		void basicTests() {
			core::List<T> a;
			for(int i = 0; i != 93; i++) {
				a.append(T(((i + 1) * (i - 7)) / 13 + i));
			}
			core::List<T> b = a.filtered([](const T &i) -> bool { return i % 2; });
			test(b.forall([](const T &i) -> bool { return i % 2; }), true, "Filter/forall test failed");
			a.filter([](const T &i) -> bool { return i % 2; });
			test(b, a, "filter/== test failed");
			b.map([](const T &i) -> T { return i * 3 + 2; });
			test(b, a.mapped([](const T &i) -> T { return i * 3 + 2; }), "Map test failed");

			core::List<T> c({1, 2, 3, 4, 10, 11, 11});
			core::List<T> d({5, 6, 7, 8, 9});
			core::List<T> e({7, 8, 6, 9});
			core::List<T> t({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 11});
			test(c.isSorted() && d.isSorted() && !e.isSorted(), true, "Sorted test failed");
			auto it = c.find(10);
			test(it != c.end() || *it == 10, true, "Find test failed");
			auto w = c.insert(range(d.begin(), d.end()), it);
			test(c, t, "Insert test failed");
			test(*w, T(10), "Insert test failed");
		}
};

} //test
} //n

