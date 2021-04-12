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

#include "format.h"

#include <y/core/String.h>

#include <algorithm>
#include <memory>

namespace y {
namespace detail {

static constexpr usize fmt_total_buffer_size = (fmt_max_size + 1) * 4;
static thread_local std::unique_ptr<char[]> fmt_buffer = nullptr;
static thread_local char* fmt_buffer_end = nullptr;

FmtBuffer::FmtBuffer() {
    if(!fmt_buffer) {
        fmt_buffer = std::make_unique<char[]>(fmt_total_buffer_size + 1);
        fmt_buffer_end = fmt_buffer.get();
        fmt_buffer[fmt_total_buffer_size] = 0;
    }
    _start = _buffer = fmt_buffer_end;
    _buffer_size = std::min(usize(fmt_buffer.get() + fmt_total_buffer_size - _start), fmt_max_size);

    y_debug_assert(_start >= fmt_buffer.get());
    y_debug_assert(_start <= fmt_buffer.get() + fmt_total_buffer_size);
    y_debug_assert(_start + _buffer_size <= fmt_buffer.get() + fmt_total_buffer_size);
}

FmtBuffer::FmtBuffer(core::String& str) {
    _dynamic = &str;
    usize cap = str.capacity();
    usize size = str.size();
    _buffer_size = cap - size;
    Y_TODO(this makes fmt_into really slow)
    str.grow(cap, 0);
    _start = _buffer = str.data() + size;
}

std::string_view FmtBuffer::done() && {
    y_debug_assert(!fmt_buffer || fmt_buffer[fmt_total_buffer_size] == 0);
    *_buffer = 0;
    if(_dynamic) {
        _dynamic->shrink(_buffer - _dynamic->begin());
    } else {
        y_debug_assert(_buffer <= fmt_buffer.get() + fmt_total_buffer_size);
        fmt_buffer_end = (_buffer == fmt_buffer.get() + fmt_total_buffer_size) ? fmt_buffer.get() : _buffer + 1;
    }
    return std::string_view(_start, _buffer - _start);
}


bool FmtBuffer::try_expand() {
    if(_dynamic) {
        const char* begin = _dynamic->begin();
        const usize start_offset = _start - begin;
        const usize buffer_offset = _buffer - begin;

        {
            usize cap = _dynamic->capacity();
            _dynamic->set_min_capacity(cap + cap / 2);
        }

        usize new_cap = _dynamic->capacity();
        char* new_begin = _dynamic->begin();
        _dynamic->grow(new_cap, 0);

        _start = new_begin + start_offset;
        _buffer = new_begin + buffer_offset;
        _buffer_size = new_cap - buffer_offset;
        return true;
    }


    const char* end = fmt_buffer.get() + fmt_total_buffer_size;
    if(_start == fmt_buffer.get() || usize(end - _start) >= fmt_max_size) {
        return false;
    }

    const usize size = _buffer - _start;
    _buffer = fmt_buffer.get();
    std::memmove(_buffer, _start, size);
    _start = _buffer;
    _buffer += size;
    _buffer_size = fmt_max_size - size;

    y_debug_assert(_buffer <= fmt_buffer.get() + fmt_total_buffer_size);
    y_debug_assert(_buffer + _buffer_size <= fmt_buffer.get() + fmt_total_buffer_size);

    return true;
}

void FmtBuffer::advance(usize r) {
    const usize l = std::min(_buffer_size, r);
    _buffer += l;
    _buffer_size -= l;
    y_debug_assert(_dynamic || _buffer <= fmt_buffer.get() + fmt_total_buffer_size);
    y_debug_assert(_dynamic || _buffer + _buffer_size <= fmt_buffer.get() + fmt_total_buffer_size);
}

#define y_buff_fmt_(str, t)                                                         \
    do {                                                                            \
        auto _fmt_l = std::snprintf(_buffer, _buffer_size + 1, str, t);             \
        if(_fmt_l < 0 || _buffer_size < usize(_fmt_l)) {                            \
            if(!try_expand()) {                                                     \
                advance(_fmt_l < 0 ? 0 : _fmt_l);                                   \
                break;                                                              \
            }                                                                       \
        } else {                                                                    \
            advance(_fmt_l);                                                        \
            break;                                                                  \
        }                                                                           \
    } while(true)

void FmtBuffer::copy(const char* str, usize len) {
    if(!len) {
        return;
    }

    while(true) {
        if(_buffer_size < len) {
            if(!try_expand()) {
                std::memcpy(_buffer, str, _buffer_size);
                advance(_buffer_size);
                break;
            }
        } else {
            std::memcpy(_buffer, str, len);
            advance(len);
            break;
        }
    }
}

void FmtBuffer::fmt_one(const char* str) {
    copy(str, str ? std::strlen(str) : 0);
}

void FmtBuffer::fmt_one(const core::String& str) {
    copy(str.data(), str.size());
}

void FmtBuffer::fmt_one(std::string_view str) {
    copy(str.data(), str.size());
}

void FmtBuffer::fmt_one(bool i) {
    fmt_one(int(i));
}

void FmtBuffer::fmt_one(char i) {
    copy(&i, 1);
}

void FmtBuffer::fmt_one(unsigned char i) {
    copy(reinterpret_cast<char*>(&i), 1);
}

void FmtBuffer::fmt_one(const void* p) {
    y_buff_fmt_("%p", p);
}

void FmtBuffer::fmt_one(int i) {
    y_buff_fmt_("%d", i);
}

void FmtBuffer::fmt_one(long int i) {
    y_buff_fmt_("%ld", i);
}

void FmtBuffer::fmt_one(long long int i) {
    y_buff_fmt_("%lld", i);
}

void FmtBuffer::fmt_one(unsigned i) {
    y_buff_fmt_("%u", i);
}

void FmtBuffer::fmt_one(long unsigned i) {
    y_buff_fmt_("%lu", i);
}

void FmtBuffer::fmt_one(long long unsigned i) {
    y_buff_fmt_("%llu", i);
}

void FmtBuffer::fmt_one(float i) {
    y_buff_fmt_("%g", i);
}

void FmtBuffer::fmt_one(double i) {
    y_buff_fmt_("%lg", i);
}

#undef y_buff_fmt_

}

}

