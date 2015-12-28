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


#include <n/core/Map.h>
#include <n/core/String.h>
#include <n/test/TestTemplate.h>
#include <n/test/Test.h>

namespace n {
namespace test {

class MapTest : public TestTemplate<MapTest>
{
	public:
		MapTest() : TestTemplate<MapTest>() {
		}

		bool run() override {
			core::Map<int, core::String> map;
			core::Array<core::String> vals({"Zero", "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine"});
			map.insert(0, "Zero");
			map.insert(1, "One");
			map.insert(2, "Two");
			map.insert(3, "Three");
			map.insert(4, "Four");
			map.insert(5, "Five");
			map.insert(6, "Six");
			map[7] = "Seven";
			map.insert(8, "Eight");
			map.insert(9, "Nine");
			map.insert(1, "Two");
			test(map.size(), 10u, "Map size test failed");
			auto it = map.begin();
			test(TypeInfo<decltype(it)>::id, TypeInfo<core::Map<int, core::String>::iterator>::id, "Iterator type test failed");
			for(int i = 0; i != 10; i++) {
				test(it != map.end(), true, "Iteration test failed");
				test((*it)._1, i, "Iterator key test failed");
				test((*it)._2, vals[i], "Iterator value test failed");
				++it;
			}
			test(it, map.end(), "Iteration end test failed");
			it = map.find(4);
			test(it != map.end(), true, "Find test failed");
			test((*it)._2, "Four", "Find value test failed");
			(*it)._2 += "!";
			test((*map.find(4))._2, "Four!", "Value modification failed");
			core::Map<int, core::String> f = map.filtered([&](const core::Pair<const int, core::String> &p) -> bool { return p._1 % 2; });
			map.filter([&](const core::Pair<const int, core::String> &p) -> bool { return p._1 % 2; });
			test(map, f, "Filter test failed");
			f.clear();
			f.insert(1, "One");
			f.insert(3, "Three");
			f.insert(5, "Five");
			f.insert(7, "Seven");
			f.insert(9, "Nine");
			test(f.size(), 5u, "Clear test failed");
			test(map, f, "Filter test failed");
			test(map.get(7), "Seven", "Get test failed");
			test(map.get(8), "", "Get default test failed");
			test(map.get(8, "Eight"), "Eight", "Get default test failed");
			test(map.forall([&](const core::Map<int, core::String>::Element &e) -> bool { return e._2 == vals[e._1]; }), true, "Forall test failed");
			test(map.forall([&](const core::Map<int, core::String>::Element &e) -> bool { return e._2 == vals[e._1] && (e._1 % 2); }), true, "Forall reject test failed");
			return true;
		}

	private:
};

} // test
} // n
