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

#include <y/utils/format.h>

#include <y/core/String.h>
#include <y/core/Vector.h>
#include <y/test/test.h>
#include <y/math/Vec.h>
#include <memory>
#include <cstring>

namespace {
using namespace y;
using namespace y::core;

const char* get_long_c_str() {
    return "this is supposed to be a long string. it's used to force the creation of heap allocated strings during testing. To be long enouht it has to be at least String::MaxShortSize+1 bytes long (which usually eq 3 machine words: so 12 bytes on 32bits systems and 24 on 64bits)";
}

y_test_func("String short creation") {
    const String s = "lele";

    y_test_assert(!std::strcmp(s.data(), "lele"));
    y_test_assert(s.size() == 4);
    y_test_assert(s.capacity() == String::max_short_size);
}

y_test_func("String long creation") {
    String s = get_long_c_str();

    y_test_assert(!std::strcmp(s.data(), get_long_c_str()));
    y_test_assert(s.size() > String::max_short_size);
    y_test_assert(s.capacity() >= s.size());
    y_test_assert(s.is_long());
    y_test_assert(s.size() == strlen(get_long_c_str()));
}

y_test_func("String copy") {
    auto s = String();
    y_test_assert(s.size() == 0 && s.is_empty() && !s.is_long() && s.capacity() == String::max_short_size && s.data() && !*s.data());

    {
        const auto short_str = String(get_long_c_str(), String::max_short_size);
        y_test_assert(!short_str.is_long());
        s = short_str;
    }
    y_test_assert(!s.is_long());

    {
        const auto long_str = String(get_long_c_str(), String::max_short_size + 1);
        y_test_assert(long_str.is_long());
        s = long_str;
    }
    y_test_assert(s.is_long());

    s = get_long_c_str();
    const String s2 = s;
    y_test_assert(s == s2);
}

y_test_func("String add") {
    const char* c_str = get_long_c_str();
    auto a = String(c_str, 3);
    y_test_assert(a.capacity() >= 6);

    a += String(c_str + 3, 3);

    y_test_assert(a.size() == 6);
    y_test_assert(!strncmp(a.data(), c_str, a.size()));
    y_test_assert(!a.is_long());

    a += c_str + 6;
    y_test_assert(a.is_long());
    y_test_assert(a.size() == strlen(c_str));
    y_test_assert(!std::strcmp(a.data(), c_str));

    a = "a string";
    a += "another string";
    y_test_assert((a + 4) == "a stringanother string4");
}

y_test_func("String from") {
    {
        const auto s = core::String() + 125;
        y_test_assert(!std::strcmp(s.data(), "125"));
    }
    {
        const auto s = core::String() + "flubudu";
        y_test_assert(!std::strcmp(s.data(), "flubudu"));
    }
    /*{
        auto s = core::String() + 3.1415f;
        y_test_assert(!std::strcmp(s, "3.1415"));
    }
    {
        auto s = core::String() + 2.71828;
        y_test_assert(!std::strcmp(s, "2.71828"));
    }*/
}

y_test_func("String find") {
    const auto s = String(get_long_c_str());

    const auto found = s.find("strings");
    const auto end = s.find("flubudu");

    y_test_assert(found != s.end());
    y_test_assert(end == s.end());
}

y_test_func("String find") {
    const auto s = String(get_long_c_str());

    const auto found = s.find("strings");
    const auto end = s.find("flubudu");

    y_test_assert(found != s.end());
    y_test_assert(end == s.end());
}

y_test_func("fmt empty") {
    const char* base_str = "blah blah blah % blah";
    const char* f = fmt(base_str).data();

    y_test_assert(!std::strcmp(base_str, f));
}

y_test_func("fmt simple") {
    const char* f = fmt("blah blah blah % blah", 57).data();

    y_test_assert(!std::strcmp("blah blah blah 57 blah", f));
}

y_test_func("fmt at end") {
    const char* f = fmt("some string%", 42).data();

    y_test_assert(!std::strcmp("some string42", f));
}

y_test_func("fmt multi") {
    const char* f = fmt("% % % %", usize(9), -1, "pwet", String("flubudu")).data();

    y_test_assert(!std::strcmp("9 -1 pwet flubudu", f));
}

y_test_func("fmt vec") {
    const char* f = fmt("% %", math::Vec3ui(1, 2, 3), "pwet").data();

    y_test_assert(!std::strcmp("[1, 2, 3] pwet", f));
}

y_test_func("fmt vector") {
    const Vector<int> a = {1, 2, 3};
    const Vector<int> b;
    const char* f = fmt("% %", a, b).data();

    y_test_assert(!std::strcmp("[1, 2, 3] []", f));
}

y_test_func("fmt long") {
    for(usize i = 0; i != 64; ++i) {
        const usize size = 16 * fmt_max_size;
        const auto long_str = std::make_unique<char[]>(size + 1);
        std::memset(long_str.get(), 'a', size);
        long_str[size] = 0;

        const char* f = fmt("%", long_str.get()).data();

        const usize f_len = std::strlen(f);
        y_test_assert(f_len > 0 && f_len < size);
        y_test_assert(f_len == fmt_max_size);
        y_test_assert(!std::memcmp(f, long_str.get(), f_len));
    }
}

y_test_func("fmt_into simple") {
    String s = "some core::String";
    const char* f = fmt_into(s, " % % %", 1, "more str", -7).data();

    y_test_assert(s == "some core::String 1 more str -7");
    y_test_assert(!std::strcmp(f, " 1 more str -7"));
}

y_test_func("fmt twice") {
    const char* f1 = fmt("%%", 7, 'a').data();
    const char* f2 = fmt("fmt_twice % %", "pls").data();

    y_test_assert(!std::strcmp(f1, "7a"));
    y_test_assert(!std::strcmp(f2, "fmt_twice pls %"));
}

y_test_func("fmt empty") {
    const char* f = fmt("% %", 19, "").data();
    y_test_assert(!std::strcmp(f, "19 "));
}

y_test_func("fmt smalls") {
    for(usize j = 0; j != 97; ++j) {
        for(usize i = 0; i != 1037; ++i) {
            char c = i % 26 + 'a';
            switch(i % 3) {
                case 0:
                    fmt("0");
                break;

                case 1:
                    fmt("1%", c);
                break;

                case 2:
                    fmt("% %", c, char(std::toupper(c)));
                break;

            }
        }
    }
    y_test_assert(true);
}
}

