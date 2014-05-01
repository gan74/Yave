/*******************************
Copyright (C) 2013-2014 grÃ©goire ANGERAND

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

#include <n/core/String.h>
#include <n/test/TestTemplate.h>
#include <n/test/Test.h>

namespace n {
namespace test {

class StringTest : public TestTemplate<StringTest>
{
	public:
		StringTest() : TestTemplate<StringTest>() {
		}

		bool run() override {
			basicTests();
			return true;
		}

	private:
		void basicTests() {
			core::String str = "floup, flup, flap, flip";
			test(str.replaced(",", ""), "floup flup flap flip", "Replaced test");
			test(str.replaced("p", "w"), "flouw, fluw, flaw, fliw", "Replaced test 2");
			test(str.replaced("floup", "flip"), "flip, flup, flap, flip", "Replaced test 3");
			str.replace(",", "");
			core::String s = str;
			test(s.toChar(), str.toChar(), "Sharing test");
			str.replace("p ", "_");
			str.replace("flou", "flew");
			test(str == "flew_flu_fla_flip" && s == "floup flup flap flip", true, "Replace test");
			str.clear();
			test(str == "" && s == "floup flup flap flip", true, "Clear test");
			str = s;
			test(s.toChar(), str.toChar(), "Sharing 2 test");
			test(str == "floup flup flap flip" && s == "floup flup flap flip", true, "Affectation test");
			char const *cp = str.toChar();
			str.detach();
			test(str.toChar() != cp, true, "detach test");
			test(str, "floup flup flap flip", "detach test");
			test(str.size(), 20, "detach test");
			test(str.size(), strlen(str.toChar()), "detach test");
			str.map([](char c) { return c == 'f' ? 'F' : c; });
			test(str, "Floup Flup Flap Flip", "Map test");
			test(s, "floup flup flap flip", "Map test");
			test(s.mapped([](char c) { return c == 'f' ? 'F' : c; }), str, "Mapped test");
			str.filter([](char c) { return c != ' '; });
			test(str.endWith("pFlapFlip"), true, "endWith test");
			test(str.endWith("Flap"), false, "endWith test 2");
			test(str.beginWith("gné ?"), false, "beginWith test");
			test(str.beginWith("Floup"), true, "beginWith test 2");
			test(str.endWith(""), true, "endWith \"\" test");
			test(str, "FloupFlupFlapFlip", "Filter test");
			test(s, "floup flup flap flip", "Filter test");
			test(s.mapped([](char c) { return c == 'f' ? 'F' : c; }).filtered([](char c) { return c != ' '; }), str, "Filtered test");
			core::String a;
			test(str.endWith(""), true, "\"\".endWith test");
			test(a.size(), 0, "Lenght test");
			core::String b = a;
			a = a + 'c';
			test(a.isShared(), false, "Shared test");
			test(b.size(), 0, "CoW test");
			b = a;
			test(a.isShared() && b.isShared(), true, "Shared test 2");
			a += a;
			test(a.isShared() || b.isShared(), false, "Shared test 3");
			test(b.size(), 1, "CoW test 2");
			test(a, "cc", "CoW test 3");
			test(a + 3, "cc3", "Int concat test");
			test(core::String("") + 9.5f, "9.5", "Float contructor test");
			test(core::String("a") + 9.5f, "a9.5", "Float concat test");
			test(9.5f + core::String("f"), "9.5f", "Float concat test 2");
		}
};

} //test
} //n
