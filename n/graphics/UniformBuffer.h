/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

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

#ifndef N_GRAPHICS_UNIFORMBUFFER
#define N_GRAPHICS_UNIFORMBUFFER

#include "GL.h"
#include <n/core/SmartPtr.h>

namespace n {
namespace graphics {


namespace internal {
	uint maxUboBytes();
}

class DynamicBufferBase
{
	public:
		struct Data : NonCopyable
		{
			Data(uint si, BufferTarget tpe);
			~Data();

			void update(bool forceBind = false) const;

			const BufferTarget type;
			uint size;
			void *buffer;
			mutable gl::Handle handle;
			mutable bool modified;
		};


		DynamicBufferBase(uint si, BufferTarget tpe);
		DynamicBufferBase();

		void update(bool force = false);

		bool isModified() const {
			return data->modified;
		}

	protected:
		friend class ShaderInstance;
		core::SmartPtr<Data> data;

};

template<typename T, BufferTarget Type>
class TypedDynamicBuffer : public DynamicBufferBase
{
	static_assert(TypeInfo<T>::isPod, "TypedDynamicBufferBase<T> should only contain pod");

	public:
		typedef T * iterator;
		typedef T const * const_iterator;

		uint getSize() const {
			return data->size / sizeof(T);
		}

		TypedDynamicBuffer(uint si) : DynamicBufferBase(si * sizeof(T), Type) {
		}

		T &operator[](uint i) {
			data->modified = true;
			return ((T *)data->buffer)[i];
		}

		const T &operator[](uint i) const {
			return ((T *)data->buffer)[i];
		}

		iterator begin() {
			data->modified = true;
			return iterator((T *)data->buffer);
		}

		iterator end() {
			data->modified = true;
			return iterator(((T *)data->buffer) + getSize());
		}

		const_iterator begin() const {
			return iterator((T *)data->buffer);
		}

		const_iterator end() const {
			return const_iterator(((T *)data->buffer) + getSize());
		}
};

template<typename T>
class UniformBuffer : public TypedDynamicBuffer<T, UniformArrayBuffer>
{
	public:
		UniformBuffer(uint s) : TypedDynamicBuffer<T, UniformArrayBuffer>(std::min(getMaxSize(), s)) {
		}

		static uint getMaxSize() {
			return internal::maxUboBytes() / sizeof(T);
		}

};

}
}


#endif // N_GRAPHICS_UNIFORMBUFFER

