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

    _free_blocks << FreeBlock {
        0, default_vertex_count,
        0, default_triangle_count
    };

    _buffer_data = std::make_unique<MeshBufferData>();

    _buffer_data->_parent = this;
    _buffer_data->_triangle_buffer = _triangle_buffer;

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
        _buffer_data->_attrib_buffers.positions             = attrib_sub_buffers[0];
        _buffer_data->_attrib_buffers.normals_tangents      = attrib_sub_buffers[1];
        _buffer_data->_attrib_buffers.uvs                   = attrib_sub_buffers[2];
    }
}

MeshAllocator::~MeshAllocator() {
    const auto lock = y_profile_unique_lock(_lock);

    sort_and_compact_blocks();

    y_always_assert(_free_blocks.size() == 1, "Not all mesh memory has been released: mesh heap fragmented");
}

MeshDrawData MeshAllocator::alloc_mesh(core::Span<PackedVertex> vertices, core::Span<IndexedTriangle> triangles) {
    y_profile();

    const u64 triangle_count = triangles.size();
    const u64 vertex_count = vertices.size();

    MeshDrawData mesh_data;
    mesh_data._buffer_data = _buffer_data.get();
    mesh_data._vertex_count = vertex_count;
    mesh_data._indirect_data.instanceCount = 1;
    mesh_data._indirect_data.indexCount = u32(triangles.size() * 3);

    auto& global_triangle_buffer = _triangle_buffer;
    auto& global_attrib_buffer = _attrib_buffer;

    const auto [vertex_begin, triangle_begin] = alloc_block(vertex_count, triangle_count);

    y_always_assert(triangle_begin + triangle_count <= global_triangle_buffer.size(), "Triangle buffer pool is full");
    y_always_assert(vertex_begin + vertex_count <= global_attrib_buffer.byte_size() / sizeof(PackedVertex), "Vertex buffer pool is full");

    {
        CmdBufferRecorder recorder = create_disposable_cmd_buffer();

        {
            MutableTriangleSubBuffer triangle_buffer(global_triangle_buffer, triangle_count * sizeof(IndexedTriangle), triangle_begin * sizeof(IndexedTriangle));
            Mapping::stage(triangle_buffer, recorder, triangles.data());
            mesh_data._indirect_data.firstIndex = u32(triangle_begin * 3);
        }

        {
            const auto attribs_sub_buffers = _buffer_data->untyped_attrib_buffers();
            const u64 buffer_elem_count = _buffer_data->attrib_buffer_elem_count();
            {
                const u8* vertex_data = static_cast<const u8*>(static_cast<const void*>(vertices.data()));

                u64 offset = 0;
                for(const AttribSubBuffer& sub_buffer : attribs_sub_buffers) {
                    const u64 elem_size = sub_buffer.byte_size() / buffer_elem_count;
                    const u64 byte_len = vertex_count * elem_size;
                    const u64 byte_offset = sub_buffer.byte_offset() + vertex_begin * elem_size;
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

            mesh_data._indirect_data.vertexOffset = u32(vertex_begin);
        }

        loading_command_queue().submit(std::move(recorder));
    }

    return mesh_data;
}

void MeshAllocator::recycle(MeshDrawData* data) {
    const auto lock = y_profile_unique_lock(_lock);

    _free_blocks << FreeBlock {
        u64(data->_indirect_data.vertexOffset),
        data->_vertex_count,
        u64(data->_indirect_data.firstIndex) / 3,
        u64(data->_indirect_data.indexCount) / 3,
    };

    data->_indirect_data = {};
    data->_buffer_data = nullptr;

    _should_compact = true;
}


std::pair<u64, u64> MeshAllocator::alloc_block(u64 vertex_count, u64 triangle_count) {
    const auto lock = y_profile_unique_lock(_lock);

    sort_and_compact_blocks();

    for(auto& block : _free_blocks) {
        if(block.vertex_count < vertex_count || block.triangle_count < triangle_count) {
            continue;
        }

        const u64 vertex_offset = block.vertex_offset;
        const u64 triangle_offset = block.triangle_offset;

        block.vertex_offset += vertex_count;
        block.vertex_count -= vertex_count;
        block.triangle_offset += triangle_count;
        block.triangle_count -= triangle_count;

        return {vertex_offset, triangle_offset};
    }

    y_fatal("Unable to alloc mesh data");
}

void MeshAllocator::sort_and_compact_blocks() {
    y_profile();

    y_debug_assert(!_lock.try_lock());

    const usize block_count = _free_blocks.size();
    if(!_should_compact || block_count < 2) {
        return;
    }

    _should_compact = false;

    std::sort(_free_blocks.begin(), _free_blocks.end(), [](const auto& a, const auto& b) {
        return a.vertex_offset < b.vertex_offset;
    });

    usize dst = 0;
    for(usize i = 1; i != block_count; ++i) {
        const u64 dst_end = _free_blocks[dst].vertex_offset + _free_blocks[dst].vertex_count;
        if(_free_blocks[i].vertex_offset == dst_end) {
            y_debug_assert(_free_blocks[dst].triangle_offset + _free_blocks[dst].triangle_count == _free_blocks[i].triangle_offset);
            _free_blocks[dst].vertex_count += _free_blocks[i].vertex_count;
            _free_blocks[dst].triangle_count += _free_blocks[i].triangle_count;
        } else {
            ++dst;
            _free_blocks[dst] = _free_blocks[i];
        }
    }

    while(_free_blocks.size() != dst + 1) {
          _free_blocks.pop();
    }
}


std::pair<u64, u64> MeshAllocator::available() const {
    const auto lock = y_profile_unique_lock(_lock);
    u64 vert = 0;
    u64 tris = 0;
    for(const auto& b : _free_blocks) {
        vert += b.vertex_count;
        tris += b.triangle_count;
    }
    return {vert, tris};
}

std::pair<u64, u64> MeshAllocator::allocated() const {
    const auto [vert, tris] = available();
    return {default_vertex_count - vert, default_triangle_count - tris};
}

usize MeshAllocator::free_blocks() const {
    const auto lock = y_profile_unique_lock(_lock);
    return _free_blocks.size();
}

}
