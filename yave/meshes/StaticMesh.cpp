/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#include "StaticMesh.h"
#include "MeshData.h"

#include <yave/graphics/device/MeshAllocator.h>

namespace yave {

StaticMesh::StaticMesh(const MeshData& mesh_data) :
    _draw_data(mesh_allocator().alloc_mesh(mesh_data.vertices(), mesh_data.triangles())),
    _aabb(mesh_data.aabb())  {
}

bool StaticMesh::is_null() const {
    return _draw_data.triangle_buffer.is_null();
}

TriangleSubBuffer StaticMesh::triangle_buffer() const {
    return _draw_data.triangle_buffer;
}

VertexSubBuffer StaticMesh::vertex_buffer() const {
    return _draw_data.vertex_buffer;
}

const VkDrawIndexedIndirectCommand& StaticMesh::indirect_data() const {
    return _draw_data.indirect_data;
}

float StaticMesh::radius() const {
    return _aabb.origin_radius();
}

const AABB& StaticMesh::aabb() const {
    return _aabb;
}


}

