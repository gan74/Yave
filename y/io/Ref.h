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
#ifndef Y_IO_REF_H
#define Y_IO_REF_H

#include <y/utils.h>
#include "Reader.h"
#include "Writer.h"

namespace y {
namespace io {

template<typename T>
class Ref {

	public:
		Ref() = default;

		Ref(const Ref& ref) : _ptr(ref._ptr), _owned(false) {
		}

		template<typename Derived, typename = std::enable_if_t<std::is_base_of_v<T, Derived>>>
		Ref(Derived&& x) : _ptr(new Derived(std::move(x))), _owned(true) {
		}

		template<typename Derived, typename = std::enable_if_t<std::is_base_of_v<T, Derived>>>
		Ref(Derived& x) : _ptr(&x), _owned(false) {
		}

		~Ref() {
			if(_owned) {
				delete _ptr;
			}
		}

		Ref(Ref&& other) {
			swap(other);
		}

		Ref& operator=(Ref&& other) {
			swap(other);
			return *this;
		}

		Ref& operator=(const Ref& other) {
			if(_owned) {
				delete _ptr;
			}
			_ptr = other._ptr;
			_owned = false;
			return *this;
		}

		operator T&() {
			return *_ptr;
		}

		operator const T&() const {
			return *_ptr;
		}

		T* operator->() {
			return _ptr;
		}

		const T* operator->() const {
			return _ptr;
		}

	private:
		void swap(Ref& other) {
			std::swap(_ptr, other._ptr);
			std::swap(_owned, other._owned);
		}

		// may be owner or not, depending on constructor (that's why we got _owned)
		T* _ptr = nullptr;
		bool _owned = false;

};

using ReaderRef = Ref<Reader>;
using WriterRef = Ref<Writer>;

}
}

#endif // Y_IO_REF_H
