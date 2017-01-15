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
#ifndef Y_IO_DECODER_H
#define Y_IO_DECODER_H

#include "Ref.h"
#include <y/core/Result.h>
#include <y/core/Vector.h>
#include <y/core/String.h>

namespace y {
namespace io {

class Decoder {

	/*class Decode : NonCopyable {
		public:
			Decode(Decode&&) = delete;

			template<typename T>
			operator T() const && {
				return _dec.decode<T>().unwrap();
			}

		private:
			friend class Decoder;

			Decode(Decoder& d) : _dec(d) {
			}

			Decoder& _dec;
	};*/

	template<typename T>
	using is_readable = bool_type<std::is_fundamental<T>::value ||
								  std::is_pod<T>::value ||
								  std::is_trivially_copyable<T>::value>;

	public:
		Decoder(const ReaderRef& r) : _inner(r) {
		}

		Decoder(ReaderRef&& r) : _inner(std::move(r)) {
		}

		template<typename T>
		core::Result<T> decode() {
			T t;
			if(decode(t, is_readable<T>())) {
				return core::Ok(std::move(t));
			}
			return core::Err();
		}

		/*Decode decode() {
			return Decode(*this);
		}*/

	private:
		template<typename T>
		bool decode(T& t, std::true_type) {
			return _inner->read(&t, sizeof(T)).is_ok();
		}

		template<typename T, typename... Args>
		bool decode(core::Vector<T, Args...>& t, std::false_type) {
			auto c = decode<u32>();
			if(c.is_error()) {
				return false;
			}
			u32 cap = c.unwrap();
			t.set_min_capacity(cap);
			while(cap--) {
				auto e = decode<T>();
				if(e.is_error()) {
					return false;
				}
				t.push_back(e.unwrap());
			}
			return true;
		}


		ReaderRef _inner;
};

}
}

#endif // DECODER_H
