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

#include "String1.h"
#include "Array.h"
#include <string>
#include <iomanip>
#include <cstring>

namespace n {
namespace core {

typedef String1::CounterType SCType;

N_FORCE_INLINE uint sizeForStrAlloc(uint s) {
	uint mod = s % sizeof(uint);
	return s + (mod ? sizeof(uint) - mod : 0);
}

N_FORCE_INLINE char *allocStr(SCType **count, uint s) {
	if(!s) {
		return 0;
	}
	uint allocSize = sizeForStrAlloc(s);
	byte *cptr = (byte *)malloc(allocSize + sizeof(uint) + sizeof(SCType));
	if((uint)cptr % sizeof(SCType)) {
		fatal("Unaligned alloc (this should not happen");
	}
	*count = new(cptr) SCType(1);
	new(cptr + sizeof(SCType)) uint(allocSize);
	return (char *)(cptr + sizeof(SCType) + sizeof(uint));
}

N_FORCE_INLINE void freeStr(SCType **count, char *) {
	if(!*count) {
		return;
	}
	((SCType *)(*count))->~SCType();
	free(*count);
}

N_FORCE_INLINE char *reallocStr(SCType **count, uint s) {
	if(!s) {
		freeStr(count, 0);
		*count = 0;
		return 0;
	}
	SCType *cptr = *count;
	if(!cptr) {
		return allocStr(count, s);
	}
	uint allocated = *(uint *)(cptr + 1);
	uint optAllocSize =  sizeForStrAlloc(s);
	if(s > allocated || allocated + 2 * sizeof(uint) < optAllocSize) {
		cptr = (SCType *)safeRealloc(cptr, optAllocSize + sizeof(uint) + sizeof(SCType));
		*(uint *)(cptr + 1) = optAllocSize;
	}
	*count = cptr;
	return ((char *)cptr) + sizeof(SCType) + sizeof(uint);
}

String1::String1() : length(0), count(0), data(0) {
}

String1::String1(const char *cst) : String1() {
	if(cst) {
		data = allocStr(&count, length = strlen(cst));//(char *)malloc(length * sizeof(char));
		memcpy(data, cst, length * sizeof(char));
	}
}

String1::String1(const char *cst, uint l) : length(l), count(0), data(0) {
	if(l) {
		data = allocStr(&count, length);//(char *)malloc(length * sizeof(char));
		if(cst) {
			memcpy(data, cst, length * sizeof(char));
		} else {
			*data = '\0';
		}
	}
}

String1::String1(const String1 &str) : length(str.length), count(str.count), data(str.data) {
	if(count) {
		++(*count);
	}
}

String1::String1(String1 &&str) : String1() {
	swap(str);
}

String1::~String1() {
	if(data) {
		if(isUnique()) {
			freeStr(&count, data);
		} else {
			--(*count);
		}
	}
}

String1::String1(const String1 &str, uint beg, uint len) : length(len), count(str.count), data(str.data + beg) {
	if(count) {
		++(*count);
	}
}

void String1::replace(const String1 &oldS, const String1 &newS) {
	operator=(replaced(oldS, newS));
}

void String1::replace(uint beg, uint len, const String1 &newS) {
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

String1 String1::replaced(const String1 &oldS, const String1 &newS) const {
	Array<String1> concat;
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
	return String1(concat);
}

String1 String1::replaced(uint beg, uint len, const String1 &newS) const {
	return String1(Array<String1>({subString(0, beg), newS, subString(beg + len)}));
}

void String1::clear() {
	detach(0);
}

uint String1::size() const {
	return length;
}

bool String1::isEmpty() const {
	return !length;
}

bool String1::isNull() const {
	return !data;
}

char const *String1::toChar() const {
	if(!data) {
		return (char *)&null;
	}
	if(!data[length]) {
		return data;
	}
	detach(length + 1);
	length--;
	data[length] = '\0';
	return data;
}

uint String1::find(char c, uint from) const {
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

uint String1::find(const String1 &str, uint from) const {
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

bool String1::contains(char c) const {
	return find(c) < length;
}

bool String1::contains(const String1 &str) const {
	return find(str) < length;
}

String1 String1::subString(uint beg, uint len) const {
	if(!len) {
		return String1();
	}
	return String1(*this, beg, len);
}

String1 String1::subString(uint beg) const {
	return subString(beg, length - beg);
}

bool String1::beginsWith(const String1 &s) const {
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

bool String1::endsWith(const String1 &s) const {
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
	for(uint i = s.length; i; i--) {
		if(data[offset + i - 1] != s.data[i - 1]) {
			return false;
		}
	}
	return true;
}

void String1::detach() {
	detach(length);
}

void String1::swap(String1 &str) {
	uint l = str.length;
	char *ch = str.data;
	SCType *c = str.count;
	str.length = length;
	str.data = data;
	str.count = count;
	count = c;
	data = ch;
	length = l;
}

Array<String1> String1::split(const String1 &str, bool empties) const {
	Array<String1> arr;
	uint p = -1;
	uint from = 0;
	while((p = find(str, from)) != (uint)-1) {
		arr.append(subString(from, p - from));
		from = p + str.size();
	}
	arr.append(subString(from));
	return empties ? arr : arr.filtered([](const String1 &s) { return !s.isEmpty(); });
}

String1 String1::toLower() const {
	return mapped([](char c) -> char { return tolower(c); });
}

String1 String1::toUpper() const {
	return mapped([](char c) -> char { return toupper(c); });
}

String1 String1::trimmed() const {
	if(isEmpty()) {
		return *this;
	}
	uint t = 0;
	uint si = size();
	while(isspace(data[t])) {
		if(++t == si) {
			return "";
		}
	}
	uint e = 0;
	while(isspace(data[si - e - 1])) {
		e++;
	}
	return subString(t, (si - e) - t);
}

std::string String1::toStdString() const {
	return std::string(data, length);
}

String1 &String1::operator+=(const String1 &s) {
	uint tl = length;
	uint ol = s.length;
	detach(tl + ol);
	memcpy(data + tl, s.data, ol * sizeof(char));
	return *this;
}

String1 String1::operator+(const String1 &s) const {
	if(isEmpty()) {
		return s;
	}
	if(s.isEmpty()) {
		return *this;
	}
	String1 str(0, length + s.length);
	memcpy(str.data, data, length * sizeof(char));
	memcpy(str.data + length, s.data, s.length * sizeof(char));
	return str;
}

bool String1::operator==(const String1 &str) const {
	if(str.length != length) {
		return false;
	}
	if(str.data == data) {
		return str.length == length;
	}
	for(uint i = 0; i != length; i++) {
		if(data[i] != str.data[i]) {
			return false;
		}
	}
	return true;
}

bool String1::operator==(const char *str) const {
	if(strlen(str) != length) {
		return false;
	}
	for(uint i = 0; i != length; i++) {
		if(data[i] != str[i]) {
			return false;
		}
	}
	return true;
}

bool String1::operator!=(const String1 &str) const {
	return !operator==(str);
}

bool String1::operator!=(const char *str) const {
	return !operator==(str);
}

String1 &String1::operator=(const String1 &s) {
	detach(0);
	data = s.data;
	count = s.count;
	length = s.length;
	if(count) {
		(*count)++;
	}
	return *this;
}


String1 &String1::operator=(String1 &&s) {
	swap(s);
	return *this;
}

bool String1::operator<(const String1 &s) const {
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

bool String1::operator>(const String1 &s) const {
	if(data && s.data) {
		for(uint i = 0, min = std::min(length, s.length); i != min; i++) {
			if(data[i] > s.data[i]) {
				return true;
			} else if(data[i] < s.data[i]) {
				return false;
			}
		}
	}
	return length > s.length;
}

String1::operator const char *() const {
	return toChar();
}

String1::const_iterator String1::begin() const {
	return data;
}

String1::const_iterator String1::end() const {
	return data + length;
}

uint64 String1::getHash() const {
	return data ? hash(data, length) : hash(&null, 1);
}

void String1::detach(uint s) const {
	if(s) {
		if(isUnique()) {
			data = reallocStr(&count, s);
		} else {
			--(*count);
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

bool String1::isUnique() const {
	return (!count || *count <= 1);
}

bool String1::isSharedSubset() const {
	return count && ((char *)count) + sizeof(SCType) + sizeof(uint)	!= data;
}

bool String1::isShared() const {
	return count && *count > 1;
}

}
}

std::istream &operator>>(std::istream &s, n::core::String1 &str) {
	std::string st;
	s>>st;
	str = n::core::String1(st.c_str());
	return s;
}

std::ostream &operator<<(std::ostream &s, const n::core::String1 &str) {
	s<<str.toStdString();
	return s;
}




