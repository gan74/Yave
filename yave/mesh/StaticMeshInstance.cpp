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
#include "StaticMeshInstance.h"

namespace yave {

StaticMeshInstance::StaticMeshInstance(DevicePtr dptr, const MeshData& m) :
		/*triangle_buffer(allocator.create_buffer<BufferUsage::IndexBuffer>(m.triangles)),
		vertex_buffer(allocator.create_buffer<BufferUsage::VertexBuffer>(m.vertices))*/
		triangle_buffer(dptr, m.triangles),
		vertex_buffer(dptr, m.vertices) {

	indirect.vertexCount = vertex_buffer.size();
	indirect.instanceCount = 1;

	indirect.firstInstance = 0;
	indirect.firstVertex = 0;
}

StaticMeshInstance::StaticMeshInstance(StaticMeshInstance&& other) : StaticMeshInstance() {
	swap(other);
}

StaticMeshInstance& StaticMeshInstance::operator=(StaticMeshInstance&& other) {
	swap(other);
	return *this;
}

void StaticMeshInstance::swap(StaticMeshInstance& other) {
	std::swap(triangle_buffer, other.triangle_buffer);
	std::swap(vertex_buffer, other.vertex_buffer);
	std::swap(indirect, other.indirect);
}

}
