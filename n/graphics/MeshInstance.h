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

#ifndef N_GRAPHICS_MESHINSTANCE
#define N_GRAPHICS_MESHINSTANCE

#include "TriangleBuffer.h"
#include "Material.h"
#include <n/assets/Asset.h>

namespace n {
namespace graphics {

namespace internal {
	template<typename T = float>
	struct MeshInstance : core::NonCopyable
	{
		MeshInstance(const typename TriangleBuffer<T>::FreezedTriangleBuffer &&b, const graphics::Material<T> &m) : buffer(b), vao(0), material(m) {
		}

		~MeshInstance() {
			delete vao;
		}

		void draw(uint instances = 1, uint beg = 0, uint end = 0) const {
			if(!vao) {
				vao = new VertexArrayObject<T>(buffer);
			}
			material.bind();
			vao->draw(instances, beg, end);
		}


		typename TriangleBuffer<T>::FreezedTriangleBuffer buffer;
		mutable VertexArrayObject<T> *vao;
		graphics::Material<T> material;
	};
}

template<typename T = float>
class MeshInstance : private assets::Asset<internal::MeshInstance<T>>
{
	friend class MeshLoader;
	public:
		MeshInstance() :  assets::Asset<internal::MeshInstance<T>>() {
		}

		MeshInstance(const typename TriangleBuffer<T>::FreezedTriangleBuffer &&b) : MeshInstance(new internal::MeshInstance<T>(b)) {
		}

		bool isValid() const {
			return assets::Asset<internal::MeshInstance<T>>::isValid();
		}

		bool isNull() const {
			return assets::Asset<internal::MeshInstance<T>>::isNull();
		}

		T getRadius() const {
			const internal::MeshInstance<T> *i = getInternal();
			return i ? i->buffer.radius : 0;
		}

		Material<T> getMaterial() const {
			const internal::MeshInstance<T> *i = getInternal();
			return i ? i->material : Material<T>();
		}

		void draw(uint instances = 1, uint beg = 0, uint end = 0) const {
			const internal::MeshInstance<T> *i = getInternal();
			if(i) {
				i->draw(instances, beg, end);
			}
		}

	private:
		MeshInstance(const assets::Asset<internal::MeshInstance<T>> &t) : assets::Asset<internal::MeshInstance<T>>(t) {
		}

		MeshInstance(internal::MeshInstance<T> *i) : assets::Asset<internal::MeshInstance<T>>(std::move(i)) {
		}

		const internal::MeshInstance<T> *getInternal() const {
			return isValid() ? &(this->operator*()) : (const internal::MeshInstance<T> *)0;
		}
};

}
}


#endif // N_GRAPHICS_MESHINSTANCE

