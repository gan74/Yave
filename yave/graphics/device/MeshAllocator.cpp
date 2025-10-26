/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include <yave/graphics/device/DebugUtils.h>

#include <y/core/FixedArray.h>
#include <y/utils/memory.h>

namespace yave {



MeshBufferArray::MeshBufferArray() : DescriptorArray(VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
}

MeshBufferArray::Indices MeshBufferArray::create_buffers(const MeshVertexStreams& streams, TransferCmdBufferRecorder& recorder) {

    auto stage_copy = [&](const SubBuffer<BufferUsage::TransferDstBit>& dst, const void* data) {
        y_debug_assert(data);
        const u64 dst_size = dst.byte_size();
        const StagingBuffer buffer(dst_size);
        std::memcpy(buffer.map_bytes(MappingAccess::WriteOnly).raw_data(), data, dst_size);
        recorder.unbarriered_copy(buffer, dst);
    };

    Indices indices = {};
    for(usize i = 0; i != stream_count; ++i) {
        const core::Span<u8> data = streams.stream_data(VertexStreamType(i));
        y_debug_assert(!data.is_empty());

        DataBuffer buffer(data.size());
        stage_copy(buffer, data.data());

#ifdef Y_DEBUG
        if(const auto* debug = debug_utils()) {
            debug->set_resource_name(buffer.vk_buffer(), fmt_c_str("Mesh vertex {} stream", vertex_stream_name(VertexStreamType(i))));
        }
#endif

        indices[i] = add_descriptor(buffer);
        _buffers.set_min_size(indices[i] + 1);
        _buffers[indices[i]] = std::move(buffer);
    }
    return indices;
}

void MeshBufferArray::remove_buffers(Indices indices) {
    for(usize i = 0; i != stream_count; ++i) {
        remove_descriptor(indices[i]);
        _buffers[indices[i]] = {};
    }
}




MeshAllocator::MeshAllocator() :
        _attrib_buffer(default_vertex_count * MeshVertexStreams::total_vertex_size),
        _triangle_buffer(default_triangle_count) {


    _free_blocks << FreeBlock {
        0, default_vertex_count,
        0, default_triangle_count
    };

    const u64 vertex_capacity = _attrib_buffer.byte_size() / u64(MeshVertexStreams::total_vertex_size);

    _mesh_buffers = std::make_unique<MeshDrawBuffers>();

    _mesh_buffers->_parent = this;
    _mesh_buffers->_triangle_buffer = _triangle_buffer;
    _mesh_buffers->_vertex_count = usize(vertex_capacity);

    {
        u64 attrib_offset = 0;
        for(usize i = 0; i != MeshDrawBuffers::vertex_stream_count; ++i) {
            const u64 byte_len = vertex_capacity * vertex_stream_element_size(VertexStreamType(i));
            _mesh_buffers->_attrib_buffers[i] = MutableAttribSubBuffer(_attrib_buffer, byte_len, attrib_offset);
            attrib_offset += byte_len;
        }
    }

#ifdef Y_DEBUG
    if(const auto* debug = debug_utils()) {
        debug->set_resource_name(_triangle_buffer.vk_buffer(), "Mesh allocator triangle buffer");
        debug->set_resource_name(_attrib_buffer.vk_buffer(), "Mesh allocator attrib buffer");
    }
#endif
}

MeshAllocator::~MeshAllocator() {
    const auto lock = std::unique_lock(_lock);

    sort_and_compact_blocks();

    y_always_assert(_free_blocks.size() == 1, "Not all mesh memory has been released: mesh heap fragmented");
}

MeshDrawData MeshAllocator::alloc_mesh(const MeshVertexStreams& streams, core::Span<IndexedTriangle> triangles) {
    y_profile();

    const u64 triangle_count = triangles.size();
    const u64 vertex_count = streams.vertex_count();

    y_debug_assert(triangle_count);
    y_debug_assert(vertex_count);

    MeshDrawData mesh_data;
    mesh_data._mesh_buffers = _mesh_buffers.get();
    mesh_data._vertex_count = u32(vertex_count);
    mesh_data._command.index_count = u32(triangles.size() * 3);

    auto& global_triangle_buffer = _triangle_buffer;
    auto& global_attrib_buffer = _attrib_buffer;

    const auto [vertex_begin, triangle_begin] = alloc_block(vertex_count, triangle_count);

    y_always_assert(triangle_begin + triangle_count <= global_triangle_buffer.size(), "Triangle buffer pool is full");
    y_always_assert(vertex_begin + vertex_count <= global_attrib_buffer.byte_size() / sizeof(PackedVertex), "Vertex buffer pool is full");


    TransferCmdBufferRecorder recorder = create_disposable_transfer_cmd_buffer();

    auto stage_copy = [&](const SubBuffer<BufferUsage::TransferDstBit>& dst, const void* data) {
        y_debug_assert(data);
        const u64 dst_size = dst.byte_size();
        const StagingBuffer buffer(dst_size);
        std::memcpy(buffer.map_bytes(MappingAccess::WriteOnly).raw_data(), data, dst_size);
        recorder.unbarriered_copy(buffer, dst);
    };

    {
        MutableTriangleSubBuffer triangle_buffer(global_triangle_buffer, triangle_count * sizeof(IndexedTriangle), triangle_begin * sizeof(IndexedTriangle));
        stage_copy(triangle_buffer, triangles.data());
        mesh_data._command.first_index = u32(triangle_begin * 3);
    }

    {
        const auto region = recorder.region("Mesh upload");

        const auto attribs_sub_buffers = _mesh_buffers->_attrib_buffers;
        const u64 buffer_elem_count = u64(_mesh_buffers->_vertex_count);
        {
            y_debug_assert(buffer_elem_count);
            for(usize i = 0; i != attribs_sub_buffers.size(); ++i) {
                const AttribSubBuffer& sub_buffer = attribs_sub_buffers[i];
                const u64 elem_size = sub_buffer.byte_size() / buffer_elem_count;
                const u64 byte_len = vertex_count * elem_size;
                y_debug_assert(sub_buffer.byte_offset() % elem_size == 0);
                const u64 byte_offset = sub_buffer.byte_offset() + vertex_begin * elem_size;

                stage_copy(
                    SubBuffer<BufferUsage::TransferDstBit>(global_attrib_buffer, byte_len, byte_offset),
                    streams.data(VertexStreamType(i))
                );
            }
        }
    }



    {
        y_profile_zone("uploading mesh data");

        const auto lock = std::unique_lock(_lock);

        if(_free.is_empty()) {
            const usize new_size = std::max(1024_uu, _mesh_datas.size() * 2);
            TypedDataBuffer<shader::StaticMeshData> new_mesh_datas(new_size);
            recorder.unbarriered_copy(_mesh_datas, SubBuffer<BufferUsage::TransferDstBit>(new_mesh_datas, _mesh_datas.byte_size(), 0));
            for(usize i = new_mesh_datas.size(); i != _mesh_datas.size(); --i) {
                _free << u32(i - 1);
            }
            _mesh_datas = std::move(new_mesh_datas);
        }

        const auto indices = _mesh_buffer_array.create_buffers(streams, recorder);

        TypedStagingBuffer<shader::StaticMeshData> staging(1);
        staging.map(MappingAccess::WriteOnly)[0] = shader::StaticMeshData {
            indices[usize(VertexStreamType::Position)],
            indices[usize(VertexStreamType::NormalTangent)],
            indices[usize(VertexStreamType::Uv)],
            u32(999999)
        };

        const u32 index = _free.pop();

        const u64 item_size = sizeof(shader::StaticMeshData);
        recorder.unbarriered_copy(staging, SubBuffer<BufferUsage::TransferDstBit>(_mesh_datas, item_size, item_size * index));

        mesh_data.mesh_data_index = index;
    }

    recorder.submit_async();

    return mesh_data;
}

void MeshAllocator::recycle(MeshDrawData* data) {
    y_debug_assert(data->_mesh_buffers == _mesh_buffers.get());
    y_debug_assert(data->mesh_data_index != u32(-1));

    const auto lock = std::unique_lock(_lock);

    _free << data->mesh_data_index;

    _free_blocks << FreeBlock {
        u64(data->_command.vertex_offset),
        data->_vertex_count,
        u64(data->_command.first_index) / 3,
        u64(data->_command.index_count) / 3,
    };

    data->_command = {};
    data->_mesh_buffers = nullptr;

    _should_compact = true;
}


std::pair<u64, u64> MeshAllocator::alloc_block(u64 vertex_count, u64 triangle_count) {
    const auto lock = std::unique_lock(_lock);

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

    _free_blocks.shrink_to(dst + 1);
}


std::pair<u64, u64> MeshAllocator::available() const {
    const auto lock = std::unique_lock(_lock);
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
    const auto lock = std::unique_lock(_lock);
    return _free_blocks.size();
}

const MeshDrawBuffers& MeshAllocator::mesh_buffers() const {
    return *_mesh_buffers;
}

const MeshBufferArray& MeshAllocator::mesh_buffer_array() const {
    return _mesh_buffer_array;
}

SubBuffer<BufferUsage::StorageBit> MeshAllocator::mesh_data_buffer() const {
    return _mesh_datas;
}

}
