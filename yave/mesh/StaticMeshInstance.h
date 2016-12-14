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
#ifndef YAVE_MESH_STATICMESHINSTANCE_H
#define YAVE_MESH_STATICMESHINSTANCE_H

#include <yave/yave.h>

#include <yave/buffer/TypedBuffer.h>

#include "MeshData.h"

namespace yave {

template<MemoryFlags Flags = MemoryFlags::DeviceLocal>
using TriangleBuffer = TypedBuffer<IndexedTriangle, BufferUsage::IndexBuffer,Flags>;

template<MemoryFlags Flags = MemoryFlags::DeviceLocal>
using VertexBuffer = TypedBuffer<Vertex, BufferUsage::VertexBuffer, Flags>;

class StaticMeshInstance : NonCopyable {

	public:
		StaticMeshInstance() = default;
		StaticMeshInstance(DevicePtr dptr, const MeshData& m);

		StaticMeshInstance(StaticMeshInstance&& other);
		StaticMeshInstance& operator=(StaticMeshInstance&& other);

		TriangleBuffer<> triangle_buffer;
		VertexBuffer<> vertex_buffer;

		vk::DrawIndirectCommand indirect;

	private:
		void swap(StaticMeshInstance& other);

};
}

#endif // YAVE_MESH_STATICMESHINSTANCE_H
