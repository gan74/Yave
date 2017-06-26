/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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

#include "SkinnedMeshInstance.h"

namespace yave {

static const core::Vector<SkinWeights>& extract_skin(const MeshData& mesh_data) {
	if(!mesh_data.skeleton) {
		fatal("Mesh has no skeleton.");
	}
	return mesh_data.skeleton->skin;
}

SkinnedMeshInstance::SkinnedMeshInstance(DevicePtr dptr, const MeshData& mesh_data) :
		_triangle_buffer(dptr, mesh_data.triangles),
		_vertex_buffer(dptr, mesh_data.vertices),
		_skin_buffer(dptr, extract_skin(mesh_data)),
		_indirect_data(mesh_data.triangles.size() * 3, 1) {
}

SkinnedMeshInstance::SkinnedMeshInstance(SkinnedMeshInstance&& other) {
	swap(other);
}

SkinnedMeshInstance& SkinnedMeshInstance::operator=(SkinnedMeshInstance&& other) {
	swap(other);
	return *this;
}

const TriangleBuffer<>& SkinnedMeshInstance::triangle_buffer() const {
	return _triangle_buffer;
}

const VertexBuffer<>& SkinnedMeshInstance::vertex_buffer() const {
	return _vertex_buffer;
}

const TypedAttribBuffer<SkinWeights>& SkinnedMeshInstance::skin_buffer() const {
	return _skin_buffer;
}

const vk::DrawIndexedIndirectCommand& SkinnedMeshInstance::indirect_data() const {
	return _indirect_data;
}

float SkinnedMeshInstance::radius() const {
	return _radius;
}

void SkinnedMeshInstance::swap(SkinnedMeshInstance& other) {
	std::swap(_triangle_buffer, other._triangle_buffer);
	std::swap(_vertex_buffer, other._vertex_buffer);
	std::swap(_skin_buffer, other._skin_buffer);
	std::swap(_indirect_data, other._indirect_data);
	std::swap(_radius, other._radius);
}

}
