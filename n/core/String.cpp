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

#include "String.h"
#include "Array.h"
#include <string>
#include <iomanip>
#include <cstring>

namespace n {
namespace core {

N_FORCE_INLINE uint sizeForStrAlloc(uint s) {
	uint mod = s % sizeof(uint);
	return s + (mod ? sizeof(uint) - mod : 0);
}

N_FORCE_INLINE char *allocStr(uint **count, uint s) {
	if(!s) {
		return 0;
	}
	uint allocSize = sizeForStrAlloc(s);
	uint *cptr = (uint *)malloc(allocSize + 2 * sizeof(uint));
	*cptr = 1;
	cptr[1] = allocSize;
	*count = cptr;
	return (char *)(cptr + 2);
}


N_FORCE_INLINE void freeStr(uint **count, char *) {
	if(!*count) {
		return;
	}
	free(*count);
}

N_FORCE_INLINE char *reallocStr(uint **count, uint s) {
	if(!s) {
		freeStr(count, 0);
		*count = 0;
		return 0;
	}
	uint *cptr = *count;
	if(!cptr) {
		return allocStr(count, s);
	}
	uint allocated = cptr[1];
	uint optAllocSize =  sizeForStrAlloc(s);
	if(s > allocated || allocated + 2 * sizeof(uint) < optAllocSize) {
		//cppcheck-suppress memleakOnRealloc
		cptr = (uint *)realloc(cptr, optAllocSize + 2 * sizeof(uint));
		cptr[1] = optAllocSize;
	}
	*count = cptr;
	return (char *)(cptr + 2);
}

String::String() : length(0), count(0), data(0) {
}

String::String(const char *cst) : String() {
	if(cst) {
		data = allocStr(&count, length = strlen(cst));//(char *)malloc(length * sizeof(char));
		memcpy(data, cst, length * sizeof(char));
	}
}

String::String(const char *cst, uint l) : length(l), count(0), data(0) {
	if(l) {
		data = allocStr(&count, length);//(char *)malloc(length * sizeof(char));
		if(cst) {
			memcpy(data, cst, length * sizeof(char));
		} else {
			*data = '\0';
		}
	}
}

String::String(const String &str) : length(str.length), count(str.count), data(str.data) {
	if(count) {
		(*count)++;
	}
}

String::String(String &&str) : String() {
	swap(str);
}

String::~String() {
	if(data) {
		if(isUnique()) {
			freeStr(&count, data);
		} else {
			(*count)--;
		}
	}
}

String::String(const String &str, uint beg, uint len) : length(len), count(str.count), data(str.data + beg) {
	if(count) {
		(*count)++;
	}
}

void String::replace(const String &oldS, const String &newS) {
	operator=(replaced(oldS, newS));
}

void String::replace(uint beg, uint len, const String &newS) {
	uint ol = length;
	if(newS.size() > len) {
		detach(ol - len + newS.size());
		memmove(data + beg + newS.size(), data + beg + len, (ol - beg - len) * sizeof(char));
		memcpy(data + beg, newS.data, newS.length * sizeof(char));
	} else {
		memmove(data + beg + newS.size(), data + beg + len, (ol - beg - len) * sizeof(char));
		memcpy(data + beg, newS.data, newS.length * sizeof(char));
		detach(ol - len + newS.size());
	}
}

String String::replaced(const String &oldS, const String &newS) const {
	Array<String> concat;
	uint index = find(oldS);
	if(index != (uint)-1) {
		uint from = 0;
		uint offset = oldS.size();
		do {
			concat += subString(from, index - from);
			concat += newS;
			from = index + offset;
			index = find(oldS, from);
		} while(index != (uint)-1);
		concat += subString(from);
	} else {
		return *this;
	}
	return String(concat);
}

String String::replaced(uint beg, uint len, const String &newS) const {
	return String(Array<String>(subString(0, beg), newS, subString(beg + len)));
}

void String::clear() {
	detach(0);
}

uint String::size() const {
	return length;
}

bool String::isEmpty() const {
	return !length;
}

bool String::isNull() const {
	return !data;
}

char const *String::toChar() const {
	if(!data) {
		return (char *)&null;
	}
	detach(length + 1);
	length--;
	data[length] = '\0';
	return data;
}

uint String::find(char c, uint from) const {
	if(!(from < length)) {
		return -1;
	}
	for(uint i = from; i != length; i++) {
		if(data[i] == c) {
			return i;
		}
	}
	return -1;
}

uint String::find(const String &str, uint from) const {
	if(from + str.size() > length) {
		return -1;
	}
	for(uint i = from, l = length - str.length; i <= l; i++) {
		bool matched = true;
		for(uint j = 0; j != str.length; j++) {
			if(data[i + j] != str.data[j]) {
				matched = false;
				break;
			}
		}
		if(matched) {
			return i;
		}
	}
	return -1;
}

bool String::contains(char c) const {
	return find(c) < length;
}

bool String::contains(const String &str) const {
	return find(str) < length;
}

String String::subString(uint beg, uint len) const {
	return String(*this, beg, len);
}

String String::subString(uint beg) const {
	return subString(beg, length - beg);
}

bool String::beginWith(const String &s) const {
	if(length < s.length) {
		return false;
	}
	if(isNull()) {
		return s.isEmpty();
	}
	if(s.isEmpty()) {
		return true;
	}
	for(uint i = 0; i != s.length; i++) {
		if(data[i] != s.data[i]) {
			return false;
		}
	}
	return true;
}

bool String::endWith(const String &s) const {
	if(length < s.length) {
		return false;
	}
	if(isNull()) {
		return s.isEmpty();
	}
	if(s.isEmpty()) {
		return true;
	}
	uint offset = length - s.length;
	for(uint i = s.length - 1; i; i--) {
		if(data[offset + i] != s.data[i]) {
			return false;
		}
	}
	return true;
}

void String::detach() {
	detach(length);
}

void String::swap(String &str) {
	uint l = str.length;
	char *ch = str.data;
	uint *c = str.count;
	str.length = length;
	str.data = data;
	str.count = count;
	count = c;
	data = ch;
	length = l;
}

Array<String> String::split(const String &str) const {
	Array<String> arr;
	uint p = -1;
	uint from = 0;
	while((p = find(str, from)) != (uint)-1) {
		arr.append(subString(from, p - from));
		from = p + str.size();
	}
	arr.append(subString(from));
	return arr.filtered([](const String &s) { return !s.isEmpty(); });
}

String String::toLower() const {
	return mapped([](char c) -> char { return tolower(c); });
}

String String::toUpper() const {
	return mapped([](char c) -> char { return toupper(c); });
}

std::string String::toStdString() const {
	return std::string(data, length);
}

String &String::operator+=(const String &s) {
	uint tl = length;
	uint ol = s.length;
	detach(tl + ol);
	memcpy(data + tl, s.data, ol * sizeof(char));
	return *this;
}

String String::operator+(const String &s) const {
	if(isEmpty()) {
		return s;
	}
	if(s.isEmpty()) {
		return *this;
	}
	String str(0, length + s.length);
	memcpy(str.data, data, length * sizeof(char));
	memcpy(str.data + length, s.data, s.length * sizeof(char));
	return str;
}

bool String::operator==(const String &str) const {
	if(str.length != length) {
		return false;
	}
	if(count && count == str.count) {
		return true;
	}
	for(uint i = 0; i != length; i++) {
		if(data[i] != str.data[i]) {
			return false;
		}
	}
	return true;
}

bool String::operator==(const char *str) const {
	for(uint i = 0; i != length; i++) {
		if(data[i] != str[i]) {
			return false;
		}
	}
	return true;
}

bool String::operator!=(const String &str) const {
	return !operator==(str);
}

bool String::operator!=(const char *str) const {
	return !operator==(str);
}

String &String::operator=(const String &s) {
	detach(0);
	data = s.data;
	count = s.count;
	length = s.length;
	if(count) {
		(*count)++;
	}
	return *this;
}


String &String::operator=(String &&s) {
	swap(s);
	return *this;
}

bool String::operator<(const String &s) const {
	if(data && s.data) {
		for(uint i = 0, min = std::min(length, s.length); i != min; i++) {
			if(data[i] < s.data[i]) {
				return true;
			} else if(data[i] > s.data[i]) {
				return false;
			}
		}
	}
	return length < s.length;
}

String::operator const char *() const {
	return toChar();
}

String::const_iterator String::begin() const {
	return data;
}

String::const_iterator String::end() const {
	return data + length;
}

uint String::getHash() const {
	return data ? hash(data, length) : hash(&null, 1);
}

void String::detach(uint s) const {
	if(s) {
		if(isUnique() && !isSharedSubset()) {
			data = reallocStr(&count, s);
		} else {
			(*count)--;
			char *d = allocStr(&count, s);
			memcpy(d, data, std::min(length, s) * sizeof(char));
			data = d;
		}
	} else {
		if(isUnique()) {
			freeStr(&count, data);//free(data);
		}
		count = 0;
		data = 0;
	}
	length = s;
}

bool String::isUnique() const {
	return (!count || *count == 1);
}

bool String::isSharedSubset() const {
	return count && (char *)(count + 1)	!= data;
}

bool String::isShared() const {
	return count && *count > 1;
}

}
}

std::istream &operator>>(std::istream &s, n::core::String &str) {
	std::string st;
	s>>st;
	str = n::core::String(st.c_str());
	return s;
}

std::ostream &operator<<(std::ostream &s, const n::core::String &str) {
	s<<str.toStdString();
	return s;
}




