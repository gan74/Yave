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

String::LongData::LongData() : data(0), length(0) {
}

String::LongData::LongData(const LongData &l) : LongData(l.data, l.length) {
}

String::LongData::LongData(LongData &&l) : data(l.data), capacity(l.capacity), length(l.length) {
	l.data = 0;
}

String::LongData::LongData(const char *str, usize len) : LongData(str, compute_capacity(len), len) {
}

String::LongData::LongData(const char *str, usize cap, usize len) : data(alloc_long(cap)), capacity(cap), length(len) {
	if(str) {
		memcpy(data, str, len);
	}
	data[len] = 0;
}


// --------------------------------------------------- SHORT ---------------------------------------------------

String::ShortData::ShortData() : data{0}, length(0) {
}

String::ShortData::ShortData(const ShortData &s) {
	memcpy(this, &s, sizeof(ShortData));
}

String::ShortData::ShortData(const char *str, usize len) : length(len) {
	if(str) {
		memcpy(data, str, len);
	}
	data[len] = 0;
}

// --------------------------------------------------- ALLOC ---------------------------------------------------

char *String::alloc_long(usize capacity) {
	return new char[capacity + 1];
}

usize String::compute_capacity(usize len) {
	return DefaultVectorResizePolicy().ideal_capacity(len);
}

void String::free_long(LongData &d) {
	delete[] d.data;
}

// --------------------------------------------------- STRING ---------------------------------------------------

String::String() : s(ShortData()) {
}

String::String(const String &str) {
	if(str.is_long()) {
		new(&l) LongData(str.l);
	} else {
		new(&s) ShortData(str.s);
	}
}

String::String(String &&str) {
	if(str.is_long()) {
		new(&l) LongData(std::move(str.l));
	} else {
		new(&s) ShortData(str.s);
	}
}

String::String(const char *str) : String(str, strlen(str)) {
}

String::String(const char *str, usize len) {
	if(len > MaxShortSize) {
		new(&l) LongData(str, len);
	} else {
		new(&s) ShortData(str, len);
	}
}

String::String(const char *beg, const char *end) : String(beg, end - beg) {
}

String::~String() {
	if(is_long()) {
		free_long(l);
	}
}

usize String::size() const {
	return is_long() ? l.length : s.length;
}

usize String::capacity() const {
	return is_long() ? l.capacity : MaxShortSize;
}

bool String::is_empty() const {
	return !size();
}

bool String::is_long() const {
	return l.length.is_long;
}

void String::clear() {
	if(is_long()) {
		free_long(l);
	}
	new(&s) ShortData();
}

char *String::data() {
	return is_long() ? l.data : s.data;
}

const char *String::data() const {
	return is_long() ? l.data : s.data;
}

String::operator const char *() const {
	return data();
}

String::operator char *() {
	return data();
}

void String::swap(String &str) {
	u8 str_buffer[sizeof(ShortData)];
	memcpy(str_buffer, &str.s, sizeof(ShortData));
	memcpy(&str.s, &s, sizeof(ShortData));
	memcpy(&s, str_buffer, sizeof(ShortData));
}

String &String::operator=(const String &str) {
	if(&str != this) {
		if(is_long()) {
			free_long(l);
		}
		if(str.is_long()) {
			new(&l) LongData(str.l);
		} else {
			new(&s) ShortData(str.s);
		}
	}
	return *this;
}

String &String::operator=(String &&str) {
	swap(str);
	return *this;
}

String &String::operator+=(const String &str) {
	usize self_size = size();
	usize other_size = str.size();
	usize total_size = self_size + other_size;
	char *self_data = data();
	const char *other_data = str.data();

	if(capacity() >= total_size) {
		// in place
		memcpy(self_data + self_size, other_data, other_size);
		if(is_long()) {
			self_data[l.length = total_size] = 0;
		} else {
			self_data[s.length = total_size] = 0;
		}
	} else {
		LongData new_dat(0, total_size);
		memcpy(new_dat.data, self_data, self_size);
		memcpy(new_dat.data + self_size, other_data, other_size);
		new_dat.data[total_size] = 0;

		if(is_long()) {
			free_long(l);
		}
		memcpy(&l, &new_dat, sizeof(LongData));
	}
	return *this;
}












// --------------------------------------------------- TESTS ---------------------------------------------------

const char *get_long_c_str() {
	return "this is supposed to be a long string. it's used to force the creation of heap allocated strings during test. to be long enouht it has to be at least String::MaxShortSize+1 bytes long (which usually eq 3 machine words: so 12 bytes on 32bits systems and 24 on 64bits)";
}

y_test_func("String short creation") {
	auto s = str("lele");

	y_test_assert(!strcmp(s.data(), "lele"));
	y_test_assert(s.size() == 4);
	y_test_assert(s.capacity() == String::MaxShortSize);
}

y_test_func("String long creation") {
	auto s = str(get_long_c_str());

	y_test_assert(!strcmp(s.data(), get_long_c_str()));
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
	const char *c_str = get_long_c_str();
	auto a = str(c_str, 3);
	y_test_assert(a.capacity() >= 6);

	a += str(c_str + 3, 3);

	y_test_assert(a.size() == 6);
	y_test_assert(!strncmp(a.data(), c_str, a.size()));
	y_test_assert(!a.is_long());

	a += str(c_str + 6);
	y_test_assert(a.is_long());
	y_test_assert(a.size() == strlen(c_str));
	y_test_assert(!strcmp(a.data(), c_str));

	a += "?";
}

}
}


