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

template<typename T = float>
class MeshInstanceBase : core::NonCopyable
{
	public:
		MeshInstanceBase(const typename TriangleBuffer<T>::FreezedTriangleBuffer &&b, const graphics::Material<T> &m) : buffer(b), vao(0), material(m) {
		}

		~MeshInstanceBase() {
			delete vao;
		}

		void draw(uint instances = 1, uint beg = 0, uint end = 0) const {
			if(!vao) {
				vao = new VertexArrayObject<T>(buffer);
			}
			material.bind();
			vao->draw(instances, beg, end);
		}

		const Material<T> &getMaterial() const {
			return material;
		}

		T getRadius() const {
			return buffer.radius;
		}

	private:
		typename TriangleBuffer<T>::FreezedTriangleBuffer buffer;
		mutable VertexArrayObject<T> *vao;
		Material<T> material;
};

namespace internal {
	template<typename T = float>
	struct MeshInstance : core::NonCopyable
	{
		typedef typename core::Array<MeshInstanceBase<T> *>::const_iterator const_iterator;

		MeshInstance(const core::Array<MeshInstanceBase<T> *> &b) : bases(b), radius(0) {
			for(const MeshInstanceBase<T> *ba : bases) {
				radius = std::max(radius, ba->getRadius());
			}
		}

		~MeshInstance() {
			for(const MeshInstanceBase<T> *b : bases) {
				delete b;
			}
		}

		void draw(uint instances = 1) const {
			for(const MeshInstanceBase<T> *b : bases) {
				b->draw(instances);
			}
		}

		T getRadius() const {
			return radius;
		}

		const_iterator begin() const {
			return bases.begin();
		}

		const_iterator end() const {
			return bases.end();
		}

		private:
			core::Array<MeshInstanceBase<T> *> bases;
			T radius;

	};
}

template<typename T = float>
class MeshInstance : private assets::Asset<internal::MeshInstance<T>>
{
	friend class MeshLoader;
	public:
		typedef typename internal::MeshInstance<T>::const_iterator const_iterator;

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
			return i ? i->getRadius() : 0;
		}

		void draw(uint instances = 1) const {
			const internal::MeshInstance<T> *i = getInternal();
			if(i) {
				i->draw(instances);
			}
		}

		const_iterator begin() const {
			const internal::MeshInstance<T> *i = getInternal();
			return i ? i->begin() : 0;
		}

		const_iterator end() const {
			const internal::MeshInstance<T> *i = getInternal();
			return i ? i->end() : 0;
		}
	private:
		MeshInstance(const assets::Asset<internal::MeshInstance<T>> &t) : assets::Asset<internal::MeshInstance<T>>(t) {
		}

		MeshInstance(internal::MeshInstance<T> *i) : assets::Asset<internal::MeshInstance<T>>(std::move(i)) {
		}

		const internal::MeshInstance<T> *getInternal() const {
			return isValid() ? this->operator->() : (const internal::MeshInstance<T> *)0;
		}
};

}
}


#endif // N_GRAPHICS_MESHINSTANCE

