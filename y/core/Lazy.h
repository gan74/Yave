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
#ifndef Y_CORE_LAZY_H
#define Y_CORE_LAZY_H

#include <y/utils.h>
#include "Functor.h"
#include "Result.h"

namespace y {
namespace core {

template<typename T>
class Lazy : NonCopyable {
	using Builder = Function<T()>;
	public:
		Lazy(T&& t) : _data(Ok(std::forward<T>(t))) {
		}

		Lazy(Builder&& f) : _data(Err(std::forward<Builder>(f))) {
		}

		template<typename F>
		Lazy(F&& f) : _data(Err(Builder(std::forward<F>(f)))) {
		}

		const T& get() const {
			if(_data.is_error()) {
				_data = Ok(_data.error()());
			}
			return _data.unwrap();
		}

		T& get() {
			if(_data.is_error()) {
				_data = Ok(_data.error()());
			}
			return _data.unwrap();
		}

		operator const T&() const {
			return get();
		}

		operator T&() {
			return get();
		}

		const T& operator()() const {
			return get();
		}

		T& operator()() {
			return get();
		}

	private:
		 mutable Result<T, Builder> _data;
};

}
}

#endif // Y_CORE_LAZY_H
