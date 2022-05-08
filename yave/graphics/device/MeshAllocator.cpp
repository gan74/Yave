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

MeshAllocator::MeshAllocator() :
        _attrib_buffer(default_vertex_count * sizeof(PackedVertex)),
        _triangle_buffer(default_triangle_count) {
}

MeshDrawData MeshAllocator::alloc_mesh(core::Span<PackedVertex> vertices, core::Span<IndexedTriangle> triangles) {
    using MutableTriangleSubBuffer = SubBuffer<BufferUsage::IndexBit | BufferUsage::TransferDstBit>;
    using MutableAttribSubBuffer = SubBuffer<BufferUsage::AttributeBit | BufferUsage::TransferDstBit>;

    MeshDrawData mesh_data;
    mesh_data._indirect_data.instanceCount = 1;
    mesh_data._indirect_data.indexCount = u32(triangles.size() * 3);

    const u64 triangles_count = triangles.size();
    const u64 vertices_count = vertices.size();

#if 0
    auto& global_triangle_buffer = _triangle_buffer;
    auto& global_attrib_buffer = _attrib_buffer;

    const u64 triangles_begin = _triangle_offset.fetch_add(triangles_count);
    const u64 vertices_begin = _vertex_offset.fetch_add(vertices_count);
#else
    mesh_data._owned = std::make_unique<MeshDrawData::OwnedBuffers>();
    auto& global_triangle_buffer = (mesh_data._owned->triangle_buffer = TriangleBuffer<>(triangles.size()));
    auto& global_attrib_buffer = (mesh_data._owned->attrib_buffer = AttribBuffer<>(vertices.size() * sizeof(PackedVertex)));

    const u64 triangles_begin = 0;
    const u64 vertices_begin = 0;
#endif

    const u64 triangle_capacity = global_triangle_buffer.size();
    const u64 vertex_capacity = global_attrib_buffer.byte_size() / sizeof(PackedVertex);
    y_always_assert(triangles_begin + triangles_count <= triangle_capacity, "Triangle buffer pool is full");
    y_always_assert(vertices_begin + vertices_count <= vertex_capacity, "Vertex buffer pool is full");

    {
        CmdBufferRecorder recorder = create_disposable_cmd_buffer();

        MutableTriangleSubBuffer triangles_buffer(global_triangle_buffer, triangles_count * sizeof(IndexedTriangle), triangles_begin * sizeof(IndexedTriangle));
        Mapping::stage(triangles_buffer, recorder, triangles.data());

        const std::array attrib_descriprtors = {
            AttribDescriptor{ sizeof(PackedVertex::position) },
            AttribDescriptor{ sizeof(PackedVertex::packed_normal) + sizeof(PackedVertex::packed_tangent_sign) },
            AttribDescriptor{ sizeof(PackedVertex::uv) },
        };

        std::array<MutableAttribSubBuffer, attrib_descriprtors.size()> attrib_buffers = {};
        {
            u64 attrib_offset = 0;
            const u8* vertex_data = static_cast<const u8*>(static_cast<const void*>(vertices.data()));
            for(usize i = 0; i != attrib_descriprtors.size(); ++i) {
                const u64 byte_len = vertices_count * attrib_descriprtors[i].size;
                const u64 byte_offset = attrib_offset * vertex_capacity + vertices_begin * attrib_descriprtors[i].size;
                attrib_buffers[i] = MutableAttribSubBuffer(global_attrib_buffer, byte_len, byte_offset);
                Mapping::stage(attrib_buffers[i], recorder, vertex_data + attrib_offset, attrib_descriprtors[i].size, sizeof(PackedVertex));
                attrib_offset += attrib_descriprtors[i].size;
            }
        }

        loading_command_queue().submit(std::move(recorder));

        mesh_data._triangle_buffer = triangles_buffer;

        static_assert(attrib_descriprtors.size() == 3);
        mesh_data._attrib_buffers.positions = attrib_buffers[0];
        mesh_data._attrib_buffers.normals_tangents = attrib_buffers[1];
        mesh_data._attrib_buffers.uvs = attrib_buffers[2];
    }

    return mesh_data;
}
}
