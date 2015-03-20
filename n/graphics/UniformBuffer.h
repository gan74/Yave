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
#include <n/core/Ref.h>
#include "GLContext.h"

namespace n {
namespace graphics {

namespace internal {
	template<gl::GLenum Type>
	class DynamicBufferBase : core::NonCopyable
	{
		public:
			DynamicBufferBase(uint si) : size(si), buffer(malloc(size)), handle(0), modified(true) {
			}

			~DynamicBufferBase() {
				free(buffer);
				if(handle) {
					gl::GLuint h = handle;
					GLContext::getContext()->addGLTask([=]() {
						gl::glDeleteBuffers(1, &h);
					});
				}
			}

		protected:
			friend class graphics::ShaderCombinaison;

			template<typename T>
			friend class VertexArrayObject;

			void update(bool forceBind = false) const {
				if(modified) {
					if(!handle) {
						gl::glGenBuffers(1, (gl::GLuint *)&handle);
						gl::glBindBuffer(Type, handle);
						gl::glBufferData(Type, size, buffer, GL_DYNAMIC_DRAW);
					} else {
						gl::glBindBuffer(Type, handle);
						void *p = gl::glMapBuffer(Type, GL_WRITE_ONLY);
						if(!p) {
							gl::glBufferSubData(Type, 0, size, buffer);
						} else {
							memcpy(p, buffer, size);
							gl::glUnmapBuffer(Type);
						}
					}
					modified = false;
				} else if(forceBind) {
					gl::glBindBuffer(Type, handle);
				}
			}

			uint size;
			void *buffer;
			gl::GLuint handle;
			mutable bool modified;
	};

	template<typename T, gl::GLenum Type>
	class TypedDynamicBufferBase : public internal::DynamicBufferBase<Type>
	{
		static_assert(TypeInfo<T>::isPod, "TypedDynamicBufferBase should only contain pod");

		using DynamicBufferBase<Type>::buffer;
		using DynamicBufferBase<Type>::size;
		using DynamicBufferBase<Type>::modified;

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
					friend class TypedDynamicBufferBase;

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
					friend class TypedDynamicBufferBase;
					iterator(T *i) : it(i){
					}

					T *it;
			};


			uint getSize() const {
				return size / sizeof(T);
			}

			TypedDynamicBufferBase(uint si) : internal::DynamicBufferBase<Type>(si * sizeof(T)) {
			}

			core::Ref<T> operator[](uint i) {
				return core::Ref<T>(((T *)buffer)[i], [=]() { modified = true; });
			}

			iterator begin() {
				return iterator((T *)buffer);
			}

			iterator end() {
				return iterator(((T *)buffer) + getSize());
			}

			const_iterator begin() const {
				return iterator((T *)buffer);
			}

			const_iterator end() const {
				return const_iterator(((T *)buffer) + getSize());
			}
		protected:
			void *getBuffer() {
				return buffer;
			}
	};

}

template<typename T>
class UniformBuffer : public internal::TypedDynamicBufferBase<T, GL_UNIFORM_BUFFER>
{
	public:
		UniformBuffer(uint s) : internal::TypedDynamicBufferBase<T, GL_UNIFORM_BUFFER>(s) {
		}
};

}
}


#endif // N_GRAPHICS_UNIFORMBUFFER

