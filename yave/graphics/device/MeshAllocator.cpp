/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#include "MeshAllocator.h"

#include <yave/graphics/commands/CmdQueue.h>
#include <yave/graphics/graphics.h>

#include <y/utils/memory.h>

namespace yave {

MeshAllocator::MeshAllocator() {
}

MeshDrawData MeshAllocator::alloc_mesh(core::Span<PackedVertex> vertices, core::Span<IndexedTriangle> triangles) {
    MeshDrawData mesh_data = {};

    mesh_data.indirect_data.instanceCount = 1;
    mesh_data.indirect_data.indexCount = u32(triangles.size() * 3);

    {
#define INIT_BUFFER(buffer, size) buffer = decltype(buffer)(size)
        INIT_BUFFER(mesh_data.triangle_buffer, triangles.size());

        INIT_BUFFER(mesh_data.attrib_streams.positions, vertices.size());
        INIT_BUFFER(mesh_data.attrib_streams.normals_tangents, vertices.size());
        INIT_BUFFER(mesh_data.attrib_streams.uvs, vertices.size());
#undef INIT_BUFFER
    }

    {
        CmdBufferRecorder recorder(create_disposable_cmd_buffer());
        Y_TODO(change to implicit staging?)
        Mapping::stage(mesh_data.triangle_buffer, recorder, triangles.data());
        //Mapping::stage(mesh_data.vertex_buffer, recorder, vertices.data());

        Mapping::stage(mesh_data.attrib_streams.positions,           recorder, &vertices.data()->position,       sizeof(math::Vec3),     sizeof(PackedVertex));
        Mapping::stage(mesh_data.attrib_streams.normals_tangents,    recorder, &vertices.data()->packed_normal,  sizeof(u32) * 2,        sizeof(PackedVertex));
        Mapping::stage(mesh_data.attrib_streams.uvs,                 recorder, &vertices.data()->uv,             sizeof(math::Vec2),     sizeof(PackedVertex));

        loading_command_queue().submit(std::move(recorder));
    }


    return mesh_data;
}

/*VertexSubBuffer MeshAllocator::alloc_vertices(CmdBufferRecorder& recorder, core::Span<PackedVertex> vertices) {
    using MutableSubBuffer = SubBuffer<BufferUsage::AttributeBit | BufferUsage::TransferDstBit>;

    const auto lock = y_profile_unique_lock(_lock);

    const u64 begin = align_up_to(_vertex_byte_offset, VertexSubBuffer::byte_alignment());
    const u64 byte_size = vertices.size() * sizeof(PackedVertex);
    const u64 end = begin + byte_size;
    y_defer(_vertex_byte_offset = end);

    y_always_assert(end <= _vertex_buffer.size(), "Vertex buffer pool is full");

    MutableSubBuffer sub_buffer(_vertex_buffer, byte_size, begin);
    Mapping::stage(sub_buffer, recorder, vertices.data());
    return sub_buffer;
}

TriangleSubBuffer MeshAllocator::alloc_triangles(CmdBufferRecorder& recorder, core::Span<IndexedTriangle> triangles) {
    using MutableSubBuffer = SubBuffer<BufferUsage::IndexBit | BufferUsage::TransferDstBit>;

    const auto lock = y_profile_unique_lock(_lock);

    const u64 begin = align_up_to(_triangle_byte_offset, TriangleSubBuffer::byte_alignment());
    const u64 byte_size = triangles.size() * sizeof(IndexedTriangle);
    const u64 end = begin + byte_size;
    y_defer(_triangle_byte_offset = end);

    y_always_assert(end <= _triangle_buffer.size(), "Triangle buffer pool is full");

    MutableSubBuffer sub_buffer(_triangle_buffer, byte_size, begin);
    Mapping::stage(sub_buffer, recorder, triangles.data());
    return sub_buffer;
}*/

}
