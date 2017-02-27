/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/

#include "MeshInstancePool.h"

namespace yave {

using Cmd = vk::DrawIndexedIndirectCommand;

MeshInstancePool::MeshInstancePool(DevicePtr dptr, usize vertices, usize triangles) :
		DeviceLinked(dptr),
		_vertex_buffer(dptr, vertices),
		_triangle_buffer(dptr, triangles),
		_vertex_end(0),
		_triangle_end(0) {

}

StaticMeshInstance MeshInstancePool::create_static_mesh(const MeshData& data) {
	if(data.triangles.size() + _triangle_end > _triangle_buffer.size()) {
		fatal("Unable to allocate triangle buffer.");
	}
	if(data.vertices.size() + _vertex_end > _vertex_buffer.size()) {
		fatal("Unable to allocate vertex buffer.");
	}

	usize vertex_offset = _vertex_end;
	usize triangle_offset = _triangle_end;

	VertexSubBuffer<> verts(_vertex_buffer, _vertex_end, data.vertices.size());
	_vertex_end += data.vertices.size();

	TriangleSubBuffer<> tris(_triangle_buffer, _triangle_end, data.triangles.size());
	_triangle_end += data.triangles.size();

	std::copy(data.triangles.begin(), data.triangles.end(), tris.map().begin());
	std::copy(data.vertices.begin(), data.vertices.end(), verts.map().begin());

	return StaticMeshInstance(tris, verts, create_indirect_buffer_data(data, vertex_offset, triangle_offset), data.radius);
}

}
