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
        std::memcpy(data, str, len);
    }
    *(data + len) = 0;
}


// --------------------------------------------------- SHORT ---------------------------------------------------

String::ShortData::ShortData() : data{0}, length(0) {
}

String::ShortData::ShortData(const char* str, usize len) : length(len) {
    if(str) {
        std::memcpy(data, str, len);
    }
    *(data + len) = 0;
}

// --------------------------------------------------- ALLOC ---------------------------------------------------

char* String::alloc_long(usize capacity) {
    return new char[capacity + 1];
}

usize String::compute_capacity(usize len) {
    const usize cap = DefaultVectorResizePolicy().ideal_capacity(len + 1) - 1;
    y_debug_assert(cap >= len + 1);
    return cap;
}

void String::free_long(LongData& d) {
#ifdef Y_DEBUG
    if(d.data) {
        std::memset(d.data, 0xFE, d.length + 1);
    }
#endif
    delete[] d.data;
}

void String::free_short(ShortData& d) {
#ifdef Y_DEBUG
    std::memset(d.data, 0xFE, d.length + 1);
#endif
}

void String::free_data() {
    if(is_long()) {
        free_long(_l);
    } else {
        free_short(_s);
    }
}

// --------------------------------------------------- STRING ---------------------------------------------------

String::String() : _s(ShortData()) {
}

String::String(const String& str) {
    if(str.is_long()) {
        ::new(&_l) LongData(str._l);
    } else {
        ::new(&_s) ShortData(str._s);
    }
}


String::String(String&& str) {
    static_assert(std::is_trivially_destructible_v<ShortData>);
    static_assert(std::is_trivially_destructible_v<LongData>);

    if(str.is_long()) {
        ::new(&_l) LongData(std::move(str._l));
    } else {
        ::new(&_s) ShortData(str._s);
    }
}

String::String(const std::string& str) : String(str.data(), str.size()) {
}

String::String(std::string_view str) : String(str.data(), str.size()) {
}

String::String(const char* str) : String(str, std::strlen(str)) {
}

String::String(const char* str, usize len) {
    if(len > max_short_size) {
        ::new(&_l) LongData(str, len);
    } else {
        ::new(&_s) ShortData(str, len);
    }
}

String::String(const char* beg, const char* end) : String(beg, usize(end - beg)) {
}

String::~String() {
    free_data();
}

void String::set_min_capacity(usize cap) {
    if(cap > capacity() && cap > max_short_size) {
        usize self_size = size();

        LongData new_dat(data(), compute_capacity(cap), self_size);
        free_data();
        new(&_l) LongData(std::move(new_dat));
    }
}

usize String::size() const {
    return is_long() ? usize(_l.length) : usize(_s.length);
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
    free_data();
    ::new(&_s) ShortData();
}

void String::make_empty() {
    if(is_long()) {
        _l.length = 0;
        _l.data[0] = 0;
    } else {
        _s.length = 0;
        _s.data[0] = 0;
    }
}

void String::shrink(usize new_size) {
    if(new_size < size()) {
        if(is_long()) {
            _l.length = new_size;
        } else {
            _s.length = new_size;
        }
        data()[new_size] = 0;
    }

}

void String::grow(usize new_size, char c) {
    usize s = size();
    if(s >= new_size) {
        return;
    }

    set_min_capacity(new_size);
    if(is_long()) {
        _l.length = new_size;
    } else {
        _s.length = new_size;
    }
    char* d = data();
    std::memset(d + s, c, new_size - s);
    d[new_size] = 0;
}

void String::resize(usize new_size, char c) {
    grow(new_size, c);
}


char* String::data() {
    return is_long() ? _l.data : _s.data;
}

const char* String::data() const {
    return is_long() ? _l.data : _s.data;
}

String::iterator String::find(const char* str) {
    return const_cast<iterator>(const_this()->find(str));
}

String::const_iterator String::find(const char* str) const {
    const const_iterator found = std::strstr(data(), str);
    return found ? found : end();
}

String::iterator String::find(std::string_view str) {
    return const_cast<iterator>(const_this()->find(str));
}

String::const_iterator String::find(std::string_view str) const {
    if(const auto i = view().find(str); i != std::string_view::npos) {
        return begin() + i;
    }
    return end();
}

std::string_view String::sub_str(usize beg) const {
    return beg < size() ? std::string_view(begin() + beg, size() - beg) : std::string_view();
}

std::string_view String::sub_str(usize beg, usize len) const {
    const usize si = size();
    beg = std::min(beg, si);
    return std::string_view(begin() + beg, std::min(len, si - beg));
}

bool String::starts_with(std::string_view str) const {
    return str.size() <= size() && std::string_view(begin(), str.size()) == str;
}

bool String::ends_with(std::string_view str) const {
    const usize s = size();
    return str.size() <= s && std::string_view(begin() + s - str.size(), str.size()) == str;
}

String::operator const char*() const {
    return data();
}

String::operator char*() {
    return data();
}

void String::swap(String& str) {
    std::swap(_l, str._l);
}


std::string_view String::view() const {
    return std::string_view(data(), size());
}

String::operator std::string_view() const {
    return view();
}

String& String::operator=(const String& str) {
    if(&str != this) {
        if(capacity() > str.size()) {
            std::copy(str.begin(), str.end() + 1, data());
            if(is_long()) {
                _l.length = str.size();
            } else {
                _s.length = str.size();
            }
        } else {
            free_data();
            if(str.is_long()) {
                ::new(&_l) LongData(str._l);
            } else {
                ::new(&_s) ShortData(str._s);
            }
        }
    }
    return *this;
}

void String::push_back(char c) {
    append(&c, 1);
}

String& String::operator=(String&& str) {
    swap(str);
    return *this;
}

String& String::operator+=(const String& str) {
    return append(str.data(), str.size());
}

String& String::operator+=(const char* str) {
    return append(str, std::strlen(str));
}

String& String::operator+=(const std::string& str) {
    return append(str.data(), str.size());
}

String& String::operator+=(std::string_view str) {
    return append(str.data(), str.size());
}


String& String::append(const char* other_data, usize other_size) {
    usize self_size = size();
    const usize total_size = self_size + other_size;

    if(capacity() >= total_size) {
        // in place
        char* self_data = data();
        std::memcpy(self_data + self_size, other_data, other_size);
        if(is_long()) {
            self_data[_l.length = total_size] = 0;
        } else {
            self_data[_s.length = total_size] = 0;
        }
    } else {
        set_min_capacity(total_size);
        char* self_data = data();
        std::memcpy(self_data + self_size, other_data, other_size);
        self_data[total_size] = 0;
        _l.length = total_size;
    }
    return *this;
}

char& String::operator[](usize i) {
    return data()[i];
}

char String::operator[](usize i) const {
    return data()[i];
}

bool String::operator==(const char* str) const {
    return operator==(std::string_view(str));
}

bool String::operator!=(const char* str) const {
    return !operator==(str);
}

bool String::operator==(const String& str) const {
    return operator==(str.view());
}

bool String::operator!=(const String& str) const {
    return operator!=(str.view());
}

bool String::operator<(const String& str) const {
    return operator<(str.view());
}

bool String::operator==(std::string_view str) const {
    return size() == str.size() ? std::equal(begin(), end(), str.begin(), str.end()) : false;
}

bool String::operator!=(std::string_view str) const {
    return !operator==(str);
}

bool String::operator<(std::string_view str) const {
    return std::lexicographical_compare(begin(), end(), str.begin(), str.end());
}


std::string_view trim_left(std::string_view str) {
    for(usize i = 0; i != str.size(); ++i) {
        if(!std::isspace(str[i])) {
            return str.substr(i);
        }
    }
    return std::string_view();
}

std::string_view trim_right(std::string_view str) {
    for(usize i = 0; i != str.size(); ++i) {
        const usize index = str.size() - i - 1;
        if(!std::isspace(str[index])) {
            return str.substr(0, index + 1);
        }
    }
    return std::string_view();
}

std::string_view trim(std::string_view str) {
    return trim_left(trim_right(str));
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

