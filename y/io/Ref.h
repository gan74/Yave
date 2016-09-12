/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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
#ifndef Y_IO_REF_H
#define Y_IO_REF_H

#include <y/utils.h>
#include "Reader.h"
#include "Writer.h"

namespace y {
namespace io {

template<typename T>
class Ref {

	Y_ASSERT_SANE(T);

	public:
		Ref() : _ptr(nullptr), _owned(false) {
		}

		Ref(const Ref& ref) : _ptr(ref._ptr), _owned(false) {
		}

		template<typename Derived>
		Ref(Derived&& x) : _ptr(new Derived(std::move(x))), _owned(true) {
		}

		template<typename Derived>
		Ref(Derived& x) : _ptr(&x), _owned(false) {
		}

		~Ref() {
			if(_owned) {
				delete _ptr;
			}
		}

		Ref(Ref&& other) : Ref() {
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

		Owned<T*> operator->() {
			return _ptr;
		}

		Owned<const T*> operator->() const {
			return _ptr;
		}

	private:
		void swap(Ref& other) {
			std::swap(_ptr, other._ptr);
			std::swap(_owned, other._owned);
		}

		// may be owned or not, depending on constructor (that's why we got _owned)
		T* _ptr;
		bool _owned;

};

using ReaderRef = Ref<Reader>;
using WriterRef = Ref<Writer>;

}
}

#endif // Y_IO_REF_H
