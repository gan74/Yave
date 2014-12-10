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
			struct Convertible
			{
				operator core::String() const {
					return "this is a convertible";
				}
			};

			core::String str = "floup, flup, flap, flip";
			test(str.find(",", 6), 11u, "Find from test failed");
			test(str.replaced(",", ""), "floup flup flap flip", "Replaced test failed");
			test(str.replaced("p", "w"), "flouw, fluw, flaw, fliw", "Replaced test 2");
			test(str.replaced("floup", "flip"), "flip, flup, flap, flip", "Replaced test 3");
			str.replace(",", "");
			core::String s = str;
			//test(s.toChar(), str.toChar(), "Sharing test failed");
			str.replace("p ", "_");
			str.replace("flou", "flew");
			test(str == "flew_flu_fla_flip" && s == "floup flup flap flip", true, "Replace test failed");
			str.clear();
			test(str == "" && s == "floup flup flap flip", true, "Clear test failed");
			str = s;
			//test(s.toChar(), str.toChar(), "Sharing 2 test failed");
			test(str == "floup flup flap flip" && s == "floup flup flap flip", true, "Affectation test failed");
			//char const *cp = str.toChar();
			str.detach();
			//test(str.toChar() != cp, true, "detach test failed");
			test(str, "floup flup flap flip", "detach test failed");
			test(str.size(), 20u, "detach test failed");
			test(str.size(), strlen(str.toChar()), "detach test failed");
			str.map([](char c) { return c == 'f' ? 'F' : c; });
			test(str, "Floup Flup Flap Flip", "Map test failed");
			test(s, "floup flup flap flip", "Map test failed");
			test(s.mapped([](char c) { return c == 'f' ? 'F' : c; }), str, "Mapped test failed");
			str.filter([](char c) { return c != ' '; });
			test(str.endWith("pFlapFlip"), true, "endWith test failed");
			test(str.endWith("Flap"), false, "endWith test 2");
			test(str.beginWith("gna ?"), false, "beginWith test failed");
			test(str.beginWith("Floup"), true, "beginWith test 2");
			test(str.endWith(""), true, "endWith \"\" test failed");
			test(str, "FloupFlupFlapFlip", "Filter test failed");
			test(s, "floup flup flap flip", "Filter test failed");
			test(s.mapped([](char c) { return c == 'f' ? 'F' : c; }).filtered([](char c) { return c != ' '; }), str, "Filtered test failed");
			core::String a;
			test(str.endWith(""), true, "\"\".endWith test failed");
			test(a.size(), 0u, "Lenght test failed");
			a += "Share";
			core::String b = a;
			test(a.isShared() && b.isShared() && b == "Share", true, "Shared test 1");
			a = a + 'd';
			test(a, "Shared", "+ test failed");
			test(a.isShared() || b.isShared(), false, "Shared test failed");
			test(b, "Share", "CoW test failed");
			b = a;
			test(a.isShared() && b.isShared() && b == "Shared", true, "Shared test 2");
			a += a;
			test(a.isShared() || b.isShared(), false, "Shared test 3");
			test(b, "Shared", "CoW test 2");
			test(a, "SharedShared", "CoW test 3");
			test(a + 3, "SharedShared3", "Int concat test failed");
			test(core::String("") + 9.5f, "9.5", "Float contructor test failed");
			test(core::String("a") + 9.5f, "a9.5", "Float concat test failed");
			test(9.5f + core::String("f"), "9.5f", "Float concat test 2 failed");
			test("9."+ core::String("5") + "f", "9.5f", "char * concat test 2 failed");
			core::Array<char> arr('4', 'a', '{', 'K');
			test(core::String(arr), "4a{K", "Array test failed");
			test(core::String(Convertible()), "this is a convertible", "Conversion test failed");
		}
};

} //test
} //n
