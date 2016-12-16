/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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
#ifndef YAVE_MESH_MESHINSTANCEPOOL_H
#define YAVE_MESH_MESHINSTANCEPOOL_H

#include <yave/yave.h>

#include "StaticMeshInstance.h"

namespace yave {

class MeshInstancePool : NonCopyable, public DeviceLinked {

	public:
		MeshInstancePool(DevicePtr dptr, usize vertices = 1024 * 1024, usize triangles = 1024 * 1024);

		StaticMeshInstance create_static_mesh(const MeshData& data);

	private:
		VertexBuffer<> _vertex_buffer;
		TriangleBuffer<> _triangle_buffer;

		usize _vertex_end;
		usize _triangle_end;
};

}

#endif // YAVE_MESH_MESHINSTANCEPOOL_H
