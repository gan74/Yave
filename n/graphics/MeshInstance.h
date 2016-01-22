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
#include <n/concurrent/Future.h>

namespace n {
namespace graphics {

class SubMeshInstance
{
	public:
		SubMeshInstance();
		SubMeshInstance(const typename TriangleBuffer<>::FreezedTriangleBuffer &&b, const Material &m);
		SubMeshInstance(const SubMeshInstance &s, const graphics::Material &m);
		SubMeshInstance(const VertexArrayObject<> &b, const Material &m);

		const Material &getMaterial() const;
		float getRadius() const;
		const VertexArrayObject<> &getVertexArrayObject() const;

	private:
		Material material;
		VertexArrayObject<> vao;
};

class MeshInstance : public assets::Asset<core::Array<SubMeshInstance>>
{
	public:
		typedef AssetType::const_iterator const_iterator;

		MeshInstance();
		MeshInstance(const Asset<AssetType> &a);
		MeshInstance(const SubMeshInstance &sub);
		MeshInstance(const AssetType &subs);
		MeshInstance(AssetType *a);
		MeshInstance(const typename TriangleBuffer<>::FreezedTriangleBuffer &&b, const Material &m = Material());

		float getRadius() const;
		const_iterator begin() const;
		const_iterator end() const;
		AssetType getSubs() const;

	private:
		const AssetType *getInternal() const;
};

}
}


#endif // N_GRAPHICS_MESHINSTANCE

