/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

This code is licensed under the MIT License (MIT).

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
#ifndef Y_IO_DECODER_H
#define Y_IO_DECODER_H

#include "Ref.h"
#include "Byteorder.h"
#include <y/core/Vector.h>
#include <y/core/String.h>

namespace y {
namespace io {

class Decoder {

	Y_TODO(use exceptions for decoder)

	public:
		Decoder(const ReaderRef& r) : _inner(r) {
		}

		Decoder(ReaderRef&& r) : _inner(std::move(r)) {
		}

		template<Byteorder Order, typename T>
		bool decode(T& t) {
			static_assert(Order == system_byteorder() || std::is_fundamental<T>::value, "Only fundamental types can be read from non system byteorder");
			bool success = (sizeof(T) == _inner->read(&t, sizeof(T)));
			set_byteorder<Order>(t);
			return success;
		}

		template<Byteorder Order, typename T>
		bool decode(core::Vector<T>& vec) {
			u64 len = 0;
			Y_TRY(decode<Order>(len));
			return fill_vec<Order>(vec, len, std::is_fundamental<T>());
		}

		template<Byteorder Order, typename T, usize Size>
		bool decode(std::array<T, Size>& arr) {
			return decode_arr<Order>(arr, std::is_fundamental<T>());
		}

		template<Byteorder Order, typename T>
		bool decode(core::String& str) {
			u64 len = 0;
			Y_TRY(decode<Order>(len));
			char* str_data = new char[len + 1];
			auto read = _inner->read(str_data, len);
			str_data[read] = 0;
			str = core::str_from_owned(str_data);
			return read == len;
		}

		template<typename T>
		bool decode(T& t) {
			return decode<system_byteorder()>(t);
		}

		template<typename T>
		bool operator()(T& t) {
			return decode<system_byteorder()>(t);
		}

		template<Byteorder Order, typename T>
		bool operator()(T& t) {
			return decode(t);
		}



	private:
		template<Byteorder Order, typename T>
		bool fill_vec(core::Vector<T>& vec, u64 len, std::true_type) {
			vec = core::Vector<T>(len, T());
			Y_TRY(_inner->read(vec.begin(), len * sizeof(T)) == len * sizeof(T));
			for(auto& u : vec) {
				set_byteorder<Order>(u);
			}
			return true;
		}

		template<Byteorder Order, typename T>
		bool fill_vec(core::Vector<T>& vec, u64 len, std::false_type) {
			vec.make_empty();
			vec.set_min_capacity(len);
			for(; len; len--) {
				T t;
				Y_TRY(decode<Order>(t));
				vec << t;
			}
			return true;
		}

		template<Byteorder Order, typename T, usize Size>
		bool decode_arr(std::array<T, Size>& arr, std::true_type) {
			Y_TRY(_inner->read(&arr, Size * sizeof(T)) == Size * sizeof(T));
			for(auto& u : arr) {
				set_byteorder<Order>(u);
			}
			return true;
		}

		template<Byteorder Order, typename T, usize Size>
		bool decode_arr(std::array<T, Size>& arr, std::false_type) {
			for(auto& e : arr) {
				Y_TRY(decode<Order>(e));
			}
			return true;
		}

		ReaderRef _inner;
};

}
}

#endif // DECODER_H
