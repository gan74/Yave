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
#include "VertexArraySubObject.h"
#include "Material.h"
#include <n/assets/Asset.h>

namespace n {
namespace graphics {

class SubMeshInstance;

namespace internal {
	class MeshInstance;

	struct VaoAllocInfo;
	static void optimisedVaoAlloc(VaoAllocInfo *);
}

class SubMeshInstance
{
	public:
		SubMeshInstance(const typename TriangleBuffer<>::FreezedTriangleBuffer &b, const graphics::Material &m) : buffer(new TriangleBuffer<>::FreezedTriangleBuffer(b)), allocInfos(0), material(m) {
		}

		void draw(const VertexAttribs &attribs = VertexAttribs(), uint renderFlags = RenderFlag::None, uint instances = 1) const {
			if(vao.isNull()) {
				if(allocInfos) {
					internal::optimisedVaoAlloc(allocInfos);
				} else {
					vao = VertexArraySubObject<>(new VertexArrayObject<>(*buffer));
				}
			}
			vao.draw(material, attribs, renderFlags, instances);
		}

		const Material &getMaterial() const {
			return material;
		}

		float getRadius() const {
			return buffer->radius;
		}

		const typename TriangleBuffer<>::FreezedTriangleBuffer &getTriangleBuffer() const {
			return *buffer;
		}

		bool operator<(const SubMeshInstance &o) const {
			return vao < o.vao;
		}

	private:
		friend void internal::optimisedVaoAlloc(VaoAllocInfo *);
		friend class internal::MeshInstance;

		core::SmartPtr<typename TriangleBuffer<>::FreezedTriangleBuffer> buffer;

		internal::VaoAllocInfo *allocInfos;
		mutable VertexArraySubObject<> vao;

		Material material;
};

namespace internal {
	struct VaoAllocInfo
	{
		core::Array<SubMeshInstance *> subs;
		float radius;
	};

	static void optimisedVaoAlloc(VaoAllocInfo *infos) {
		core::Array<uint> indexes;
		core::Array<Vertex<>> vertices;
		for(SubMeshInstance *sub : infos->subs) {
			indexes += sub->getTriangleBuffer().indexes;
			vertices += sub->getTriangleBuffer().vertices;
			sub->allocInfos = 0;
		}
		core::SmartPtr<VertexArrayObject<>> vao(new VertexArrayObject<>(TriangleBuffer<>::FreezedTriangleBuffer(indexes, vertices, infos->radius)));
		uint iCount = 0;
		uint vCount = 0;
		for(SubMeshInstance *sub : infos->subs) {
			uint num = sub->getTriangleBuffer().indexes.size() / 3;
			sub->vao = VertexArraySubObject<>(vao, iCount, num, vCount, sub->getRadius());
			iCount += num;
			vCount += sub->getTriangleBuffer().vertices.size();
		}
		delete infos;
	}

	class MeshInstance : core::NonCopyable
	{
		public:
			typedef typename core::Array<SubMeshInstance *>::const_iterator const_iterator;

			MeshInstance(const core::Array<SubMeshInstance *> &b, bool optAlloc = true) : subs(b), radius(0) {
				core::Array<SubMeshInstance *> toAlloc;
				for(SubMeshInstance *sub : subs) {
					radius = std::max(radius, sub->getRadius());
					if(sub->vao.isNull() && !sub->allocInfos) {
						toAlloc.append(sub);
					}
				}
				if(optAlloc) {
					VaoAllocInfo *allocInfos = new VaoAllocInfo{toAlloc, radius};
					for(SubMeshInstance *sub : subs) {
						sub->allocInfos = allocInfos;
					}
				}
			}

			MeshInstance(const typename TriangleBuffer<>::FreezedTriangleBuffer &&b, const graphics::Material &m = graphics::Material()) : MeshInstance(core::Array<SubMeshInstance *>({new SubMeshInstance(b, m)})) {
			}

			~MeshInstance() {
				for(const SubMeshInstance *b : subs) {
					delete b;
				}
			}

			void draw(const VertexAttribs &attribs = VertexAttribs(), uint instances = 1) const {
				for(const SubMeshInstance *b : subs) {
					b->draw(attribs, instances);
				}
			}

			float getRadius() const {
				return radius;
			}

			const_iterator begin() const {
				return subs.begin();
			}

			const_iterator end() const {
				return subs.end();
			}

			const core::Array<SubMeshInstance *> &getBases() const {
				return subs;
			}

		private:
			core::Array<SubMeshInstance *> subs;
			float radius;
	};
}

class MeshInstance : private assets::Asset<internal::MeshInstance>
{
	friend class MeshLoader;
	public:
		typedef typename internal::MeshInstance::const_iterator const_iterator;

		MeshInstance() : assets::Asset<internal::MeshInstance>() {
		}

		MeshInstance(core::Array<SubMeshInstance *> &subs) : MeshInstance(new internal::MeshInstance(subs)) {
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

