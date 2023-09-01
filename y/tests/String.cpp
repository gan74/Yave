/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

y_test_func("String replace") {
    y_test_assert(core::String::replaced("abdaddaaaabdadad", "a", "x") == "xbdxddxxxxbdxdxd");
    y_test_assert(core::String::replaced("xyuyyyxyxyxyx", "a", "k") == "xyuyyyxyxyxyx");
    y_test_assert(core::String::replaced("", "a", "b") == "");
}

}

