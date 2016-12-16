/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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

#include "String.h"
#include "Vector.h"
#include <y/test/test.h>
#include <memory>
#include <cstring>

namespace y {
namespace core {
// --------------------------------------------------- LONG ---------------------------------------------------

String::LongData::LongData() : data(nullptr), capacity(0), length(0) {
}

String::LongData::LongData(const LongData& l) : LongData(l.data, l.length) {
}

String::LongData::LongData(LongData&& l) : data(l.data), capacity(l.capacity), length(l.length) {
	l.data = nullptr;
}

String::LongData::LongData(const char* str, usize len) : LongData(str, compute_capacity(len), len) {
}

String::LongData::LongData(const char* str, usize cap, usize len) : data(alloc_long(cap)), capacity(cap), length(len) {
	if(str) {
		memcpy(data, str, len);
	}
	*(data + len) = 0;
}


// --------------------------------------------------- SHORT ---------------------------------------------------

String::ShortData::ShortData() : _data{0}, _length(0) {
}

String::ShortData::ShortData(const ShortData& s) {
	memcpy(this, &s, sizeof(ShortData));
}

String::ShortData::ShortData(const char* str, usize len) : _length(len) {
	if(str) {
		memcpy(_data, str, len);
	}
	*(_data + len) = 0;
}

// --------------------------------------------------- ALLOC ---------------------------------------------------

char* String::alloc_long(usize capacity) {
	return new char[capacity + 1];
}

usize String::compute_capacity(usize len) {
	return DefaultVectorResizePolicy().ideal_capacity(len);
}

void String::free_long(LongData& d) {
	delete[] d.data;
}

// --------------------------------------------------- STRING ---------------------------------------------------

String::String() : _s(ShortData()) {
}

String::String(const String& str) {
	if(str.is_long()) {
		new(&_l) LongData(str._l);
	} else {
		new(&_s) ShortData(str._s);
	}
}

String::String(String&& str) {
	if(str.is_long()) {
		new(&_l) LongData(std::move(str._l));
	} else {
		new(&_s) ShortData(str._s);
	}
}

String::String(const char* str) : String(str, strlen(str)) {
}

String::String(const char* str, usize len) {
	if(len > MaxShortSize) {
		new(&_l) LongData(str, len);
	} else {
		new(&_s) ShortData(str, len);
	}
}

String::String(const char* beg, const char* end) : String(beg, usize(end - beg)) {
}

String::~String() {
	if(is_long()) {
		free_long(_l);
	}
}

String String::from_owned(Owner<char*> owned) {
	usize len = strlen(owned);
	String str;
	str._l.length = len;
	str._l.capacity = len;
	str._l.data = owned;
	return str;
}

usize String::size() const {
	return is_long() ? _l.length : _s._length;
}

usize String::capacity() const {
	return is_long() ? _l.capacity : MaxShortSize;
}

bool String::is_empty() const {
	return !size();
}

bool String::is_long() const {
	return _l.length._is_long;
}

void String::clear() {
	if(is_long()) {
		free_long(_l);
	}
	new(&_s) ShortData();
}

char* String::data() {
	return is_long() ? _l.data : _s._data;
}

const char* String::data() const {
	return is_long() ? _l.data : _s._data;
}

String::iterator String::find(const String& str) {
	return const_cast<iterator>(const_this()->find(str));
}

String::const_iterator String::find(const String& str) const {
	const_iterator found = strstr(data(), str);
	return found ? found : end();
}

String String::sub_str(usize beg) const {
	return beg < size() ? String(begin() + beg) : String();
}

String String::sub_str(usize beg, usize len) const {
	usize si = size();
	beg = std::min(beg, si);
	return String(begin() + beg, std::min(len, si - beg));
}

String::operator const char*() const {
	return data();
}

String::operator char*() {
	return data();
}

void String::swap(String& str) {
	u8 str_buffer[sizeof(ShortData)];
	memcpy(str_buffer, &str._s, sizeof(ShortData));
	memcpy(&str._s, &_s, sizeof(ShortData));
	memcpy(&_s, str_buffer, sizeof(ShortData));
}

String& String::operator=(const String& str) {
	if(&str != this) {
		if(is_long()) {
			free_long(_l);
		}
		if(str.is_long()) {
			new(&_l) LongData(str._l);
		} else {
			new(&_s) ShortData(str._s);
		}
	}
	return *this;
}

String& String::operator=(String&& str) {
	swap(str);
	return *this;
}

String& String::operator+=(const String& str) {
	usize self_size = size();
	usize other_size = str.size();
	usize total_size = self_size + other_size;
	char* self_data = data();
	const char* other_data = str.data();

	if(capacity() >= total_size) {
		// in place
		memcpy(self_data + self_size, other_data, other_size);
		if(is_long()) {
			self_data[_l.length = total_size] = 0;
		} else {
			self_data[_s._length = total_size] = 0;
		}
	} else {
		LongData new_dat(nullptr, total_size);
		memcpy(new_dat.data, self_data, self_size);
		memcpy(new_dat.data + self_size, other_data, other_size);
		new_dat.data[total_size] = 0;

		if(is_long()) {
			free_long(_l);
		}
		memcpy(&_l, &new_dat, sizeof(LongData));
	}
	return *this;
}

char& String::operator[](usize i) {
	return data()[i];
}

char String::operator[](usize i) const {
	return data()[i];
}

bool String::operator==(const String& str) const {
	return size() == str.size() ? std::equal(begin(), end(), str.begin(), str.end()) : false;
}

bool String::operator!=(const String& str) const {
	return !operator==(str);
}

bool String::operator<(const String& str) const {
	//return strcmp(data(), str.data()) < 0;
	return std::lexicographical_compare(begin(), end(), str.begin(), str.end());
}


usize utf8_len(char c) {
	if(c & 0x80) {
		usize len = 0;
		for(; c & 0x80; c <<= 1) {
			len++;
		}
		return len;
	}
	return 1;
}

Vector<u32> String::to_unicode() const {
	usize si = size();
	const char* dat = data();
	auto utf8 = vector_with_capacity<u32>((si * 2) / 3);
	while(dat < end()) {
		usize len = utf8_len(*dat);
		//u32 buffer = c & (0xFF >> len);
		if(len == 1) {
			utf8 << u32(*dat++);
		} else {
			u32 buffer = *dat++ & (0xFF >> len);
			for(usize l = 1; l < len; l++) {
				buffer = (buffer << 6) | (*dat++ & 0x3F);
			}
			utf8 << buffer;
		}
	}
	return utf8;
}








// --------------------------------------------------- TESTS ---------------------------------------------------

const char* get_long_c_str() {
	return "this is supposed to be a long string. it's used to force the creation of heap allocated strings during test. to be long enouht it has to be at least String::MaxShortSize+1 bytes long (which usually eq 3 machine words: so 12 bytes on 32bits systems and 24 on 64bits)";
}

y_test_func("String short creation") {
	auto s = str("lele");

	y_test_assert(!strcmp(s, "lele"));
	y_test_assert(s.size() == 4);
	y_test_assert(s.capacity() == String::MaxShortSize);
}

y_test_func("String long creation") {
	auto s = str(get_long_c_str());

	y_test_assert(!strcmp(s, get_long_c_str()));
	y_test_assert(s.size() > String::MaxShortSize);
	y_test_assert(s.capacity() >= s.size());
	y_test_assert(s.is_long());
	y_test_assert(s.size() == strlen(get_long_c_str()));
}

y_test_func("String copy") {
	auto s = str();
	y_test_assert(s.size() == 0 && s.is_empty() && !s.is_long() && s.capacity() == String::MaxShortSize && s.data() && !*s.data());

	{
		auto short_str = str(get_long_c_str(), String::MaxShortSize);
		y_test_assert(!short_str.is_long());
		s = short_str;
	}
	y_test_assert(!s.is_long());

	{
		auto long_str = str(get_long_c_str(), String::MaxShortSize + 1);
		y_test_assert(long_str.is_long());
		s = long_str;
	}
	y_test_assert(s.is_long());
}


y_test_func("String add") {
	const char* c_str = get_long_c_str();
	auto a = str(c_str, 3);
	y_test_assert(a.capacity() >= 6);

	a += str(c_str + 3, 3);

	y_test_assert(a.size() == 6);
	y_test_assert(!strncmp(a, c_str, a.size()));
	y_test_assert(!a.is_long());

	a += str(c_str + 6);
	y_test_assert(a.is_long());
	y_test_assert(a.size() == strlen(c_str));
	y_test_assert(!strcmp(a, c_str));

	a += "?";
}

y_test_func("String from_owned") {
	usize size = 28;
	char* c_str = new char[size + 1];
	memcpy(c_str, get_long_c_str(), size);
	c_str[size] = 0;

	auto str = str_from_owned(c_str);

	usize index = size / 2;
	str[index]++;
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


y_test_func("String unicode") {
	auto s = str(u8"фa€\u0444");
	y_test_assert(s.size() == 8);

	auto utf = s.to_unicode();
	y_test_assert(utf.size() == 4);
	y_test_assert(utf == vector<u32>(0x0444, 'a', 0x20AC, 0x0444));
}



}
}


