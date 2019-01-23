/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#include <y/core/Result.h>
#include "Reader.h"
#include "Writer.h"

#include <memory>


namespace y {
namespace io {

template<typename T>
class Ref {

	public:
		using Result = core::Result<void, usize>;

		Ref() = default;

		Ref(const Ref& ref) : _ref(ref._ref) {
		}

		template<typename Derived, typename = std::enable_if_t<std::is_base_of_v<T, Derived>>>
		Ref(Derived&& x) : _storage(std::make_unique<Derived>(y_fwd(x))), _ref(_storage.get()) {
			static_assert(std::is_base_of_v<T, Derived>);
		}

		template<typename Derived, typename = std::enable_if_t<std::is_base_of_v<T, Derived>>>
		Ref(Derived& x) : _ref(&x) {
			static_assert(std::is_base_of_v<T, Derived>);
		}

		Ref(Ref&& other) {
			swap(other);
		}

		Ref& operator=(Ref&& other) {
			swap(other);
			return *this;
		}

		Ref& operator=(const Ref& other) {
			_storage = nullptr;
			_ref = other._ref;
			return *this;
		}

		operator T&() {
			return *_ref;
		}

		operator const T&() const {
			return *_ref;
		}

		T* operator->() {
			return _ref;
		}

		const T* operator->() const {
			return _ref;
		}



		// for serde2, remove
		bool at_end() const {
			return _ref->at_end();
		}

		Result read(void* data, usize bytes) {
			try {
				_ref->read(data, bytes);
				return core::Ok();
			} catch(...) {
				return core::Err(usize(0));
			}
		}

		Result read_all(core::Vector<u8>& data) {
			try {
				_ref->read_all(data);
				return core::Ok();
			} catch(...) {
				return core::Err(usize(0));
			}
		}

		Result write(const void* data, usize bytes) {
			try {
				_ref->write(data, bytes);
				return core::Ok();
			} catch(...) {
				return core::Err(usize(0));
			}
		}

		void flush() {
			_ref->flush();
		}

	private:
		void swap(Ref& other) {
			std::swap(_ref, other._ref);
			std::swap(_storage, other._storage);
		}

		std::unique_ptr<T> _storage;
		T* _ref = nullptr;

};

using ReaderRef = Ref<Reader>;
using WriterRef = Ref<Writer>;

}
}

#endif // Y_IO_REF_H
