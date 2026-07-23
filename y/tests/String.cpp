/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

    y_test_assert(s.find(std::string_view("strings")) != s.end());
    y_test_assert(s.find(std::string_view("flubudu")) == s.end());
}

y_test_func("String replace") {
    y_test_assert(core::String::replaced("abdaddaaaabdadad", "a", "x") == "xbdxddxxxxbdxdxd");
    y_test_assert(core::String::replaced("xyuyyyxyxyxyx", "a", "k") == "xyuyyyxyxyxyx");
    y_test_assert(core::String::replaced("", "a", "b") == "");
}

y_test_func("String self append") {
    {
        String s = "abc";
        s += s;
        y_test_assert(s == "abcabc");
    }
    {
        String s = "xy";
        s += std::string_view(s.data() + 1, 1);
        y_test_assert(s == "xyy");
    }
    {
        String s = get_long_c_str();
        const usize original_size = s.size();
        s += s;
        y_test_assert(s.size() == original_size * 2);
        y_test_assert(s.sub_str(0, original_size) == get_long_c_str());
        y_test_assert(s.sub_str(original_size) == get_long_c_str());
    }
    {
        String s = get_long_c_str();
        const usize original_size = s.size();
        s += std::string_view(s.data() + 10, 20);
        y_test_assert(s.size() == original_size + 20);
        y_test_assert(s.sub_str(original_size) == std::string_view(get_long_c_str() + 10, 20));
    }
    {
        String s = "short";
        while(s.size() <= String::max_short_size) {
            s += s.sub_str(0, 1);
        }
        y_test_assert(s.is_long());
        y_test_assert(s.starts_with("short"));
    }
}

y_test_func("String move") {
    {
        String a = "hello";
        String b = std::move(a);
        y_test_assert(b == "hello");
    }
    {
        String a = get_long_c_str();
        String b = std::move(a);
        y_test_assert(b == get_long_c_str());
        y_test_assert(b.is_long());
    }
    {
        String a = "short";
        String b = get_long_c_str();
        a = std::move(b);
        y_test_assert(a == get_long_c_str());
        y_test_assert(a.is_long());
    }
}

y_test_func("String shrink grow") {
    String s = get_long_c_str();

    s.shrink(10);
    y_test_assert(s.size() == 10);
    y_test_assert(s == std::string_view(get_long_c_str(), 10));

    s.grow(20, 'x');
    y_test_assert(s.size() == 20);
    y_test_assert(s.sub_str(10) == "xxxxxxxxxx");

    s.resize(5, 'z');
    y_test_assert(s == "this ");
}

y_test_func("String starts ends with") {
    const String s = "hello world";
    y_test_assert(s.starts_with("hello"));
    y_test_assert(s.starts_with(""));
    y_test_assert(!s.starts_with("world"));
    y_test_assert(s.ends_with("world"));
    y_test_assert(s.ends_with(""));
    y_test_assert(!s.ends_with("hello"));
}

y_test_func("String make empty") {
    {
        String s = "abc";
        s.make_empty();
        y_test_assert(s.is_empty());
        y_test_assert(!s.is_long());
        y_test_assert(s.capacity() == String::max_short_size);
    }
    {
        String s = get_long_c_str();
        const usize cap = s.capacity();
        s.make_empty();
        y_test_assert(s.is_empty());
        y_test_assert(s.is_long());
        y_test_assert(s.capacity() == cap);
    }
    {
        String s = get_long_c_str();
        s.clear();
        y_test_assert(s.is_empty());
        y_test_assert(!s.is_long());
    }
}

y_test_func("String short long transitions") {
    String s;
    for(usize i = 0; i != String::max_short_size; ++i) {
        s.push_back(char('a' + (i % 26)));
        y_test_assert(!s.is_long());
        y_test_assert(s.size() == i + 1);
    }
    s.push_back('Z');
    y_test_assert(s.is_long());
    y_test_assert(s.size() == String::max_short_size + 1);
    y_test_assert(s[s.size() - 1] == 'Z');

    s = "x";
    y_test_assert(!s.is_long());
    y_test_assert(s == "x");

    s = get_long_c_str();
    y_test_assert(s.is_long());
    const String copy = s;
    s = "ok";
    y_test_assert(!s.is_long());
    y_test_assert(s == "ok");
    y_test_assert(copy == get_long_c_str());
}

y_test_func("String doubling stress") {
    String s = "ab";
    for(usize i = 0; i != 12; ++i) {
        const usize before = s.size();
        s += s;
        y_test_assert(s.size() == before * 2);
        y_test_assert(s.sub_str(0, before) == s.sub_str(before));
    }
    y_test_assert(s.size() == (2_uu << 12));
    y_test_assert(s.starts_with("ab"));
    y_test_assert(s.ends_with("ab"));
}

y_test_func("String replace chained") {
    String s = "foo_bar_foo_baz_foo";
    s = s.replaced("foo", "qux");
    y_test_assert(s == "qux_bar_qux_baz_qux");
    s = s.replaced("qux", "X");
    y_test_assert(s == "X_bar_X_baz_X");
    s = s.replaced("_", "");
    y_test_assert(s == "XbarXbazX");
    y_test_assert(String::replaced("aaa", "aa", "b") == "ba");
}

y_test_func("String compare and trim") {
    y_test_assert(String("abc") < String("abd"));
    y_test_assert(String("abc") < std::string_view("abd"));
    y_test_assert(!(String("abc") < String("abc")));
    y_test_assert(String("abc") == std::string_view("abc"));
    y_test_assert(String("abc") != std::string_view("abcd"));

    y_test_assert(trim("  hello  ") == "hello");
    y_test_assert(trim_left("\t\nxy") == "xy");
    y_test_assert(trim_right("xy  ") == "xy");
    y_test_assert(trim("   ") == "");
    y_test_assert(trim("") == "");
}

/*y_test_func("String copy assign capacity reuse") {
    String s = get_long_c_str();
    const usize cap = s.capacity();
    s = "tiny";
    y_test_assert(s == "tiny");
    y_test_assert(s.is_long());
    y_test_assert(s.capacity() == cap);

    String long_a = get_long_c_str();
    String long_b = String(get_long_c_str()) + "EXTRA";
    long_a.set_min_capacity(long_b.size() + 64);
    const usize reserved = long_a.capacity();
    long_a = long_b;
    y_test_assert(long_a == long_b);
    y_test_assert(long_a.capacity() == reserved);
}*/

y_test_func("String from iterators") {
    const char chars[] = {'h', 'e', 'l', 'l', 'o'};
    const String s(chars, chars + 5);
    y_test_assert(s == "hello");

    Vector<char> vec;
    for(char c : std::string_view(get_long_c_str())) {
        vec.push_back(c);
    }
    const String from_vec(vec.begin(), vec.end());
    y_test_assert(from_vec == get_long_c_str());
}

y_test_func("String append mix") {
    String s = "a";
    s += String("b");
    s += "c";
    s += std::string("d");
    s += std::string_view("e");
    s.push_back('f');
    y_test_assert(s == "abcdef");

    const String long_s = get_long_c_str();
    s += long_s;
    y_test_assert(s.starts_with("abcdef"));
    y_test_assert(s.ends_with(long_s.view()));
    y_test_assert(s.size() == 6 + long_s.size());
}

}

