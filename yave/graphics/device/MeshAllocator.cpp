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

    _buffer_data = std::make_unique<MeshBufferData>();

    _buffer_data->triangle_buffer = _triangle_buffer;

    {
        const std::array attrib_descriptors = {
            AttribDescriptor{ sizeof(PackedVertex::position) },
            AttribDescriptor{ sizeof(PackedVertex::packed_normal) + sizeof(PackedVertex::packed_tangent_sign) },
            AttribDescriptor{ sizeof(PackedVertex::uv) },
        };

        const u64 vertex_capacity = _attrib_buffer.byte_size() / sizeof(PackedVertex);
        std::array<MutableAttribSubBuffer, attrib_descriptors.size()> attrib_sub_buffers = {};
        {
            u64 attrib_offset = 0;
            for(usize i = 0; i != attrib_descriptors.size(); ++i) {
                const u64 byte_len = vertex_capacity * attrib_descriptors[i].size;
                attrib_sub_buffers[i] = MutableAttribSubBuffer(_attrib_buffer, byte_len, attrib_offset);
                attrib_offset += byte_len;
            }
        }

        static_assert(attrib_sub_buffers.size() == 3);
        _buffer_data->attrib_buffers.positions             = attrib_sub_buffers[0];
        _buffer_data->attrib_buffers.normals_tangents      = attrib_sub_buffers[1];
        _buffer_data->attrib_buffers.uvs                   = attrib_sub_buffers[2];
    }
}

MeshDrawData MeshAllocator::alloc_mesh(core::Span<PackedVertex> vertices, core::Span<IndexedTriangle> triangles) {
    MeshDrawData mesh_data;
    mesh_data._buffer_data = _buffer_data.get();
    mesh_data._indirect_data.instanceCount = 1;
    mesh_data._indirect_data.indexCount = u32(triangles.size() * 3);

    const u64 triangles_count = triangles.size();
    const u64 vertices_count = vertices.size();

    auto& global_triangle_buffer = _triangle_buffer;
    auto& global_attrib_buffer = _attrib_buffer;

    const u64 triangles_begin = _triangle_offset.fetch_add(triangles_count);
    const u64 vertices_begin = _vertex_offset.fetch_add(vertices_count);

    y_always_assert(triangles_begin + triangles_count <= global_triangle_buffer.size(), "Triangle buffer pool is full");
    y_always_assert(vertices_begin + vertices_count <= global_attrib_buffer.byte_size() / sizeof(PackedVertex), "Vertex buffer pool is full");

    {
        CmdBufferRecorder recorder = create_disposable_cmd_buffer();

        {
            MutableTriangleSubBuffer triangle_buffer(global_triangle_buffer, triangles_count * sizeof(IndexedTriangle), triangles_begin * sizeof(IndexedTriangle));
            Mapping::stage(triangle_buffer, recorder, triangles.data());
            mesh_data._indirect_data.firstIndex = u32(triangles_begin * 3);
        }

        {
            const auto attribs_sub_buffers = _buffer_data->untyped_attrib_buffers();
            const u64 buffer_elem_count = _buffer_data->attrib_buffer_elem_count();
            {
                const u8* vertex_data = static_cast<const u8*>(static_cast<const void*>(vertices.data()));

                u64 offset = 0;
                for(const AttribSubBuffer& sub_buffer : attribs_sub_buffers) {
                    const u64 elem_size = sub_buffer.byte_size() / buffer_elem_count;
                    const u64 byte_len = vertices_count * elem_size;
                    const u64 byte_offset = sub_buffer.byte_offset() + vertices_begin * elem_size;
                    Mapping::stage(
                        MutableAttribSubBuffer(global_attrib_buffer, byte_len, byte_offset),
                        recorder,
                        vertex_data + offset,   // data
                        elem_size,              // elem_size
                        sizeof(PackedVertex)    // input_stride
                    );
                    offset += elem_size;
                }
            }

            mesh_data._indirect_data.vertexOffset = u32(vertices_begin);
        }

        loading_command_queue().submit(std::move(recorder));
    }

    return mesh_data;
}
}
