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
#include <n/concurrent/Future.h>

namespace n {
namespace graphics {

enum MeshOptimisationOptions
{
	MergeVAO = 0x01,

};

namespace internal {
	class MeshInstance;
}

class SubMeshInstance
{
	public:
		SubMeshInstance(const typename TriangleBuffer<>::FreezedTriangleBuffer &b, const Material &m);
		SubMeshInstance(const SubMeshInstance &s, const graphics::Material &m);
		SubMeshInstance(const VertexArraySubObject<> &b, const Material &m);

		void draw(const VertexAttribs &attribs = VertexAttribs(), uint renderFlags = RenderFlag::None, uint instances = 1) const;
		const Material &getMaterial() const;
		float getRadius() const;
		const VertexArraySubObject<> &getVertexArrayObject() const;

	private:
		friend class internal::MeshInstance;

		SubMeshInstance(const core::SmartPtr<typename TriangleBuffer<>::FreezedTriangleBuffer> &b, const graphics::Material &m);
		void alloc() const;

		Material material;

		mutable core::SmartPtr<typename TriangleBuffer<>::FreezedTriangleBuffer> buffer;
		mutable VertexArraySubObject<> vao;
};

namespace internal {
	class MeshInstance : NonCopyable
	{
		public:
			typedef typename core::Array<SubMeshInstance *>::const_iterator const_iterator;

			MeshInstance(const core::Array<SubMeshInstance *> &b, uint opt = MeshOptimisationOptions::MergeVAO);
			MeshInstance(const typename TriangleBuffer<>::FreezedTriangleBuffer &&b, const graphics::Material &m = graphics::Material());
			~MeshInstance();

			void draw(const VertexAttribs &attribs = VertexAttribs(), uint instances = 1) const;
			float getRadius() const;
			const_iterator begin() const;
			const_iterator end() const;
			const core::Array<SubMeshInstance *> &getBases() const;

		private:
			core::Array<SubMeshInstance *> subs;
			float radius;
	};
}

class MeshInstance : public assets::Asset<internal::MeshInstance>
{
	friend class MeshLoader;
	public:
		typedef typename internal::MeshInstance::const_iterator const_iterator;

		MeshInstance();
		MeshInstance(const core::Array<SubMeshInstance *> &subs);
		MeshInstance(const typename TriangleBuffer<>::FreezedTriangleBuffer &&b, const Material &m = Material());

		float getRadius() const;
		void draw(const VertexAttribs &attribs = VertexAttribs(), uint instances = 1) const;
		const_iterator begin() const;
		const_iterator end() const;
		core::Array<SubMeshInstance *> getBases() const;

	private:
		MeshInstance(const assets::Asset<internal::MeshInstance> &t);
		MeshInstance(internal::MeshInstance *i);

		const internal::MeshInstance *getInternal() const;
};

}
}


#endif // N_GRAPHICS_MESHINSTANCE

