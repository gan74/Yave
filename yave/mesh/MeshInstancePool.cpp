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

#include "MeshInstancePool.h"

namespace yave {

using Cmd = vk::DrawIndexedIndirectCommand;

static auto create_indirect_buffer(const MeshData& m, usize offset, usize max = usize(-1)) {
	core::Vector<Cmd> cmds;
	for(usize i = 0, size = m.triangles.size(); i < size; i += max) {
		cmds << Cmd(u32(std::min(size - i, max) * 3), 1, u32(i * 3), i32(offset));
	}
	return cmds;
}

MeshInstancePool::MeshInstancePool(DevicePtr dptr, usize vertices, usize triangles) :
		DeviceLinked(dptr),
		_vertex_buffer(dptr, vertices),
		_triangle_buffer(dptr, triangles),
		_vertex_end(0),
		_triangle_end(0) {

}

StaticMeshInstance MeshInstancePool::create_static_mesh(const MeshData& data) {
	if(data.triangles.size() + _triangle_end > _triangle_buffer.size()) {
		fatal("Unable to allocate triangle buffer");
	}
	if(data.vertices.size() + _vertex_end > _vertex_buffer.size()) {
		fatal("Unable to allocate vertex buffer");
	}

	usize offset = _vertex_end;

	VertexSubBuffer<> verts(_vertex_buffer, _vertex_end, data.vertices.size());
	_vertex_end += data.vertices.size();

	TriangleSubBuffer<> tris(_triangle_buffer, _triangle_end, data.triangles.size());
	_triangle_end += data.triangles.size();

	memcpy(tris.map().begin(), data.triangles.begin(), data.triangles.size() * sizeof(IndexedTriangle));
	memcpy(verts.map().begin(), data.vertices.begin(), data.vertices.size() * sizeof(Vertex));

	return StaticMeshInstance{tris, verts, IndirectBuffer<>(get_device(), create_indirect_buffer(data, offset))};
}

}
