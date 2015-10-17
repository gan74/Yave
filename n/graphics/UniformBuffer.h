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
#include "GLContext.h"

namespace n {
namespace graphics {


class DynamicBufferBase
{
	public:
		struct Data : NonCopyable
		{
			Data(uint si, gl::GLenum tpe) : type(tpe), size(si), buffer(malloc(size)), handle(0), modified(true) {
			}

			~Data() {
				free(buffer);
				if(handle) {
					gl::GLuint h = handle;
					GLContext::getContext()->addGLTask([=]() {
						gl::glDeleteBuffers(1, &h);
					});
				}
			}

			void update(bool forceBind = false) const {
				if(modified) {
					if(!handle) {
						gl::glGenBuffers(1, (gl::GLuint *)&handle);
						gl::glBindBuffer(type, handle);
						gl::glBufferData(type, size, buffer, GL_DYNAMIC_DRAW);
					} else {
						gl::glBindBuffer(type, handle);
						/*void *p = gl::glMapBuffer(type, GL_WRITE_ONLY);
						if(!p) {
							gl::glBufferSubData(type, 0, size, buffer);
						} else {
							memcpy(p, buffer, size);
							gl::glUnmapBuffer(type);
						}*/
						gl::glBufferSubData(type, 0, size, 0);
						gl::glBufferSubData(type, 0, size, buffer);
					}
					modified = false;
				} else if(forceBind) {
					gl::glBindBuffer(type, handle);
				}
			}

			const gl::GLenum type;
			uint size;
			void *buffer;
			gl::GLuint handle;
			mutable bool modified;
		};


		DynamicBufferBase(uint si, gl::GLenum tpe) : data(new Data(si, tpe)) {
		}

		void update(bool force = false) {
			data->update(force);
		}

	protected:
		friend class ShaderInstance;
		core::SmartPtr<Data> data;

};

template<typename T, gl::GLenum Type>
class TypedDynamicBuffer : public DynamicBufferBase
{
	static_assert(TypeInfo<T>::isPod, "TypedDynamicBufferBase<T> should only contain pod");

	public:
		class const_iterator
		{
			public:
				const_iterator &operator++() { // ++prefix
					it++;
					return *this;
				}

				const_iterator &operator--() { // --prefix
					it--;
					return *this;
				}

				const_iterator &operator++(int) { // postfix++
					it++;
					return const_iterator(it - 1);
				}

				const_iterator &operator--(int) { // postfix--
					it--;
					return const_iterator(it + 1);
				}

				const T &operator*() const {
					return *it;
				}

				bool operator==(const const_iterator &i) const {
					return i.it == it;
				}

				bool operator!=(const const_iterator &i) const {
					return i.it != it;
				}

			private:
				friend class iterator;
				friend class TypedDynamicBuffer;

				const_iterator(const T *i) : it(i){
				}

				const T *it;
		};


		class iterator
		{
			public:
				iterator &operator++() { // ++prefix
					it++;
					return *this;
				}

				iterator &operator--() { // --prefix
					it--;
					return *this;
				}

				iterator &operator++(int) { // postfix++
					it++;
					return iterator(it - 1);
				}

				iterator &operator--(int) { // postfix--
					it--;
					return iterator(it + 1);
				}

				T &operator*() {
					return *it;
				}

				const T &operator*() const {
					return *it;
				}

				operator const_iterator() const {
					return const_iterator(it);
				}

				bool operator==(const const_iterator &i) const {
					return i.it == it;
				}

				bool operator!=(const const_iterator &i) const {
					return i.it != it;
				}

			private:
				friend class TypedDynamicBuffer;
				iterator(T *i) : it(i){
				}

				T *it;
		};

		uint getSize() const {
			return data->size / sizeof(T);
		}

		TypedDynamicBuffer(uint si) : DynamicBufferBase(si * sizeof(T), Type) {
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
class UniformBuffer : public TypedDynamicBuffer<T, GL_UNIFORM_BUFFER>
{
	public:
		UniformBuffer(uint s) : TypedDynamicBuffer<T, GL_UNIFORM_BUFFER>(s) {
		}
};

}
}


#endif // N_GRAPHICS_UNIFORMBUFFER

