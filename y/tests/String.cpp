/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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

#include <y/core/String.h>
#include <y/core/Vector.h>
#include <y/test/test.h>
#include <memory>
#include <cstring>

using namespace y;
using namespace y::core;

const char* get_long_c_str() {
	return "this is supposed to be a long string. it's used to force the creation of heap allocated strings during testing. To be long enouht it has to be at least String::MaxShortSize+1 bytes long (which usually eq 3 machine words: so 12 bytes on 32bits systems and 24 on 64bits)";
}

y_test_func("String short creation") {
	auto s = str("lele");

	y_test_assert(!strcmp(s, "lele"));
	y_test_assert(s.size() == 4);
	y_test_assert(s.capacity() == String::max_short_size);
}

y_test_func("String long creation") {
	auto s = str(get_long_c_str());

	y_test_assert(!strcmp(s, get_long_c_str()));
	y_test_assert(s.size() > String::max_short_size);
	y_test_assert(s.capacity() >= s.size());
	y_test_assert(s.is_long());
	y_test_assert(s.size() == strlen(get_long_c_str()));
}

y_test_func("String copy") {
	auto s = String();
	y_test_assert(s.size() == 0 && s.is_empty() && !s.is_long() && s.capacity() == String::max_short_size && s.data() && !*s.data());

	{
		auto short_str = String(get_long_c_str(), String::max_short_size);
		y_test_assert(!short_str.is_long());
		s = short_str;
	}
	y_test_assert(!s.is_long());

	{
		auto long_str = String(get_long_c_str(), String::max_short_size + 1);
		y_test_assert(long_str.is_long());
		s = long_str;
	}
	y_test_assert(s.is_long());

	s = get_long_c_str();
	String s2 = s;
	y_test_assert(s == s2);
}

y_test_func("String add") {
	const char* c_str = get_long_c_str();
	auto a = String(c_str, 3);
	y_test_assert(a.capacity() >= 6);

	a += String(c_str + 3, 3);

	y_test_assert(a.size() == 6);
	y_test_assert(!strncmp(a, c_str, a.size()));
	y_test_assert(!a.is_long());

	a += str(c_str + 6);
	y_test_assert(a.is_long());
	y_test_assert(a.size() == strlen(c_str));
	y_test_assert(!strcmp(a, c_str));

	a = "a string";
	a += "another string";
	y_test_assert((a + 4) == "a stringanother string4");
}

y_test_func("String from_owned") {
	usize size = 28;
	char* c_str = new char[size + 1];
	std::memcpy(c_str, get_long_c_str(), size);
	c_str[size] = 0;

	auto str = String::from_owned(c_str);

	usize index = size / 2;
	++str[index];
	y_test_assert(str[index] == get_long_c_str()[index] + 1);
}

y_test_func("String from") {
	{
		auto s = str(125);
		y_test_assert(!strcmp(s, "125"));
	}
	{
		auto s = str("flubudu");
		y_test_assert(!strcmp(s, "flubudu"));
	}
	{
		auto s = str(3.1415f);
		y_test_assert(!strcmp(s, "3.1415"));
	}
	{
		auto s = str(2.71828);
		y_test_assert(!strcmp(s, "2.71828"));
	}
}

y_test_func("String find") {
	auto s = str(get_long_c_str());

	auto found = s.find("strings");
	auto end = s.find("flubudu");

	y_test_assert(found != s.end());
	y_test_assert(end == s.end());
}


y_test_func("String find") {
	auto s = str(get_long_c_str());

	auto found = s.find("strings");
	auto end = s.find("flubudu");

	y_test_assert(found != s.end());
	y_test_assert(end == s.end());
}


/*y_test_func("String unicode") {
	auto s = str(u8"Ñ„aâ‚¬\u0444");
	y_test_assert(s.size() == 8);

	auto utf = s.to_unicode();
	y_test_assert(utf.size() == 4);
	y_test_assert(utf == vector<u32>(0x0444, 'a', 0x20AC, 0x0444));
}*/




