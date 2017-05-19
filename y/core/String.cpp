/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
	if(len > max_short_size) {
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
	return is_long() ? _l.capacity : max_short_size;
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


/*static usize utf8_len(char c) {
	if(c & 0x80) {
		usize len = 0;
		for(; c & 0x80; c <<= 1) {
			++len;
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
			for(usize l = 1; l < len; ++l) {
				buffer = (buffer << 6) | (*dat++ & 0x3F);
			}
			utf8 << buffer;
		}
	}
	return utf8;
}*/

}
}


