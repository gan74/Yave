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
#include "VertexAttribs.h"
#include "VertexArrayObject.h"
#include "Material.h"
#include <n/assets/Asset.h>

namespace n {
namespace graphics {

class SubMeshInstance : core::NonCopyable
{
	public:
		SubMeshInstance(const typename TriangleBuffer<>::FreezedTriangleBuffer &&b, const graphics::Material &m) : buffer(b), vao(0), material(m) {
		}

		~SubMeshInstance() {
			delete vao;
		}

		void draw(const VertexAttribs &attribs = VertexAttribs(), uint renderFlags = RenderFlag::None, uint instances = 1) const {
			if(!vao) {
				vao = new VertexArrayObject<>(buffer);
			}
			material.bind(renderFlags);
			vao->draw(attribs, instances);
		}

		const Material &getMaterial() const {
			return material;
		}

		float getRadius() const {
			return buffer.radius;
		}

		const typename TriangleBuffer<>::FreezedTriangleBuffer &getTriangleBuffer() const {
			return buffer;
		}

	private:
		typename TriangleBuffer<>::FreezedTriangleBuffer buffer;
		mutable VertexArrayObject<> *vao;
		Material material;
};

namespace internal {
	struct MeshInstance : core::NonCopyable
	{
		typedef typename core::Array<SubMeshInstance *>::const_iterator const_iterator;

		MeshInstance(const core::Array<SubMeshInstance *> &b) : bases(b), radius(0) {
			for(const SubMeshInstance *ba : bases) {
				radius = std::max(radius, ba->getRadius());
			}
		}

		MeshInstance(const typename TriangleBuffer<>::FreezedTriangleBuffer &&b, const graphics::Material &m = graphics::Material()) : MeshInstance(core::Array<SubMeshInstance *>({new SubMeshInstance(std::move(b), m)})) {
		}

		~MeshInstance() {
			for(const SubMeshInstance *b : bases) {
				delete b;
			}
		}

		void draw(const VertexAttribs &attribs = VertexAttribs(), uint instances = 1) const {
			for(const SubMeshInstance *b : bases) {
				b->draw(attribs, instances);
			}
		}

		float getRadius() const {
			return radius;
		}

		const_iterator begin() const {
			return bases.begin();
		}

		const_iterator end() const {
			return bases.end();
		}

		const core::Array<SubMeshInstance *> &getBases() const {
			return bases;
		}

		private:
			core::Array<SubMeshInstance *> bases;
			float radius;

	};
}

class MeshInstance : private assets::Asset<internal::MeshInstance>
{
	friend class MeshLoader;
	public:
		typedef typename internal::MeshInstance::const_iterator const_iterator;

		MeshInstance() :  assets::Asset<internal::MeshInstance>() {
		}

		MeshInstance(const typename TriangleBuffer<>::FreezedTriangleBuffer &&b, const Material &m = Material()) : MeshInstance(new internal::MeshInstance(std::move(b), m)) {
		}

		bool isValid() const {
			return assets::Asset<internal::MeshInstance>::isValid();
		}

		bool isNull() const {
			return assets::Asset<internal::MeshInstance>::isNull();
		}

		float getRadius() const {
			const internal::MeshInstance *i = getInternal();
			return i ? i->getRadius() : -1;
		}

		void draw(const VertexAttribs &attribs = VertexAttribs(), uint instances = 1) const {
			const internal::MeshInstance *i = getInternal();
			if(i) {
				i->draw(attribs, instances);
			}
		}

		const_iterator begin() const {
			const internal::MeshInstance *i = getInternal();
			return i ? i->begin() : 0;
		}

		const_iterator end() const {
			const internal::MeshInstance *i = getInternal();
			return i ? i->end() : 0;
		}

		core::Array<SubMeshInstance *> getBases() const {
			const internal::MeshInstance *i = getInternal();
			return i ? i->getBases() : core::Array<SubMeshInstance *>();
		}

	private:
		MeshInstance(const assets::Asset<internal::MeshInstance> &t) : assets::Asset<internal::MeshInstance>(t) {
		}

		MeshInstance(internal::MeshInstance *i) : assets::Asset<internal::MeshInstance>(std::move(i)) {
		}

		const internal::MeshInstance *getInternal() const {
			return isValid() ? this->operator->() : (const internal::MeshInstance *)0;
		}
};

}
}


#endif // N_GRAPHICS_MESHINSTANCE

