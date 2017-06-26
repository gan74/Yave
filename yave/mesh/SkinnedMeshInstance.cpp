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


SkinnedMeshInstance::SkinnedMeshInstance(DevicePtr dptr, const MeshData& mesh_data) :
		_triangle_buffer(dptr, mesh_data.triangles()),
		_vertex_buffer(dptr, mesh_data.skinned_vertices()),
		_indirect_data(mesh_data.indirect_data()),
		_skeleton(mesh_data.bones()),
		_radius(mesh_data.radius()) {
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

const SkinnedVertexBuffer<>& SkinnedMeshInstance::vertex_buffer() const {
	return _vertex_buffer;
}

const vk::DrawIndexedIndirectCommand& SkinnedMeshInstance::indirect_data() const {
	return _indirect_data;
}

const Skeleton& SkinnedMeshInstance::skeleton() const {
	return _skeleton;
}

float SkinnedMeshInstance::radius() const {
	return _radius;
}

void SkinnedMeshInstance::swap(SkinnedMeshInstance& other) {
	std::swap(_triangle_buffer, other._triangle_buffer);
	std::swap(_vertex_buffer, other._vertex_buffer);
	std::swap(_indirect_data, other._indirect_data);
	std::swap(_skeleton, other._skeleton);
	std::swap(_radius, other._radius);
}

}
