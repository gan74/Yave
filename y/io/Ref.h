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
		Ref() : ptr(nullptr), owned(false) {
		}

		Ref(const Ref &ref) : ptr(ref.ptr), owned(false) {
		}

		template<typename Derived>
		Ref(Derived &&x) : ptr(new Derived(std::move(x))), owned(true) {
		}

		template<typename Derived>
		Ref(Derived &x) : ptr(&x), owned(false) {
		}

		~Ref() {
			if(owned) {
				delete ptr;
			}
		}

		Ref(Ref &&other) : Ref() {
			swap(other);
		}

		Ref &operator=(Ref &&other) {
			swap(other);
			return *this;
		}

		Ref &operator=(const Ref &other) {
			if(owned) {
				delete ptr;
			}
			ptr = other.ptr;
			owned = false;
			return *this;
		}

		operator T&() {
			return *ptr;
		}

		operator const T&() const {
			return *ptr;
		}

		Owned<T*> operator->() {
			return ptr;
		}

		Owned<const T*> operator->() const {
			return ptr;
		}

	private:
		void swap(Ref &other) {
			std::swap(ptr, other.ptr);
			std::swap(owned, other.owned);
		}

		Owned<T*> ptr;
		bool owned;

};

using ReaderRef = Ref<Reader>;
using WriterRef = Ref<Writer>;

}
}

#endif // Y_IO_REF_H
