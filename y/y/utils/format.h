/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#ifndef Y_FORMAT_H
#define Y_FORMAT_H

#include <y/defines.h>
#include "types.h"
#include <cstring>
#include <string_view>

namespace y {

namespace core {
class String;
}

namespace detail {

class FmtBuffer {
	public:
		FmtBuffer();
		FmtBuffer(core::String& str);

		void copy(const char* str, usize len);

		void fmt_one(const char* str);
		void fmt_one(const void* p);
		void fmt_one(const core::String& str);
		void fmt_one(std::string_view str);
		void fmt_one(char i);
		void fmt_one(unsigned char i);
		void fmt_one(int i);
		void fmt_one(long int i);
		void fmt_one(long long int i);
		void fmt_one(unsigned i);
		void fmt_one(long unsigned i);
		void fmt_one(long long unsigned i);
		void fmt_one(float i);
		void fmt_one(double i);

		template<typename T, typename = std::enable_if_t<is_iterable_v<T>>>
		void fmt_one(const T& t) {
			fmt_one('[');
			usize size = t.size();
			if(size) {
				auto it = std::begin(t);
				for(usize i = 1; i < size; ++i) {
					fmt_one(*it);
					++it;
					copy(", ", 2);
				}
				fmt_one(*it);
			}
			fmt_one(']');
		}

		std::string_view done() &&;

	private:
		bool try_expand();
		void advance(usize r);

		char* _buffer = nullptr;
		usize _buffer_size = 0;
		char* _start = nullptr;
		core::String* _dynamic = nullptr;
};


template<typename T, typename... Args>
void fmt_rec(FmtBuffer& buffer, const char* fmt, T&& t, Args&&... args) {
	const char* c = std::strchr(fmt, '%');
	if(!c) {
		buffer.fmt_one(fmt);
	} else {
		buffer.copy(fmt, c - fmt);
		++c;
		buffer.fmt_one(y_fwd(t));
		if constexpr(sizeof...(args)) {
			fmt_rec(buffer, c, y_fwd(args)...);
		} else {
			buffer.fmt_one(c);
		}
	}
}

}

static constexpr usize fmt_max_size = 1023;

template<typename... Args>
std::string_view fmt(const char* fmt, Args&&... args) {
	if constexpr(sizeof...(args)) {
		detail::FmtBuffer buffer;
		detail::fmt_rec(buffer, fmt, y_fwd(args)...);
		return std::move(buffer).done();
	}

	return fmt;
}

template<typename... Args>
std::string_view fmt_into(core::String& out, const char* fmt, Args&&... args) {
	detail::FmtBuffer buffer(out);
	if constexpr(sizeof...(args)) {
		detail::fmt_rec(buffer, fmt, y_fwd(args)...);
	} else {
		buffer.fmt_one(fmt);
	}
	return std::move(buffer).done();
}

}

#endif // Y_FORMAT_H
