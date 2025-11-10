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

#include <y/utils/format.h>

namespace yave {

MeshAllocator::MeshAllocator() : _triangle_buffer(default_triangle_count) {

    _free_blocks << FreeBlock {
        0, default_triangle_count
    };

#ifdef Y_DEBUG
    if(const auto* debug = debug_utils()) {
        debug->set_resource_name(_triangle_buffer.vk_buffer(), "Mesh allocator triangle buffer");
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
    y_debug_assert(triangle_count);

    MeshDrawData mesh_data;
    mesh_data._parent = this;
    mesh_data._vertex_count = u32(streams.vertex_count());
    mesh_data._cmd.index_count = u32(triangles.size() * 3);

    const u64 triangle_begin = alloc_block(triangle_count);
    y_always_assert(triangle_begin + triangle_count <= _triangle_buffer.size(), "Triangle buffer pool is full");

    TransferCmdBufferRecorder recorder = create_disposable_transfer_cmd_buffer();

    auto staged_copy = [&](const SubBuffer<BufferUsage::TransferDstBit>& dst, const void* data) {
        y_debug_assert(data);
        const u64 dst_size = dst.byte_size();
        const StagingBuffer buffer(dst_size);
        std::memcpy(buffer.map_bytes(MappingAccess::WriteOnly).raw_data(), data, dst_size);
        recorder.unbarriered_copy(buffer, dst);
    };

    {
        /*
         * We can not simply realloc the triangle buffer as some systems call triangle_buffer unsynchronized
         * Locking in triangle_buffer might be the best option
         */
        MutableTriangleSubBuffer triangle_buffer(_triangle_buffer, triangle_count * sizeof(IndexedTriangle), triangle_begin * sizeof(IndexedTriangle));
        staged_copy(triangle_buffer, triangles.data());
        mesh_data._cmd.first_index = u32(triangle_begin * 3);
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


        const u32 index = _free.pop();
        mesh_data._mesh_data_index = index;

        _mesh_buffers.set_min_size(index + 1);

        Buffers& buffers = _mesh_buffers[index];
        for(usize i = 0; i != stream_count; ++i) {
            const core::Span<u8> data = streams.stream_data(VertexStreamType(i));
            y_debug_assert(!data.is_empty());

            mesh_data._mesh_buffers.attribs[i] = (buffers[i] = VertexBuffer(data.size()));
            staged_copy(buffers[i], data.data());

#ifdef Y_DEBUG
            if(const auto* debug = debug_utils()) {
                debug->set_resource_name(buffers[i].vk_buffer(), fmt_c_str("Mesh vertex {} stream", vertex_stream_name(VertexStreamType(i))));
            }
#endif
        }

        TypedStagingBuffer<shader::StaticMeshData> staging(1);
        staging.map(MappingAccess::WriteOnly)[0] = shader::StaticMeshData {
            buffers[usize(VertexStreamType::Position)].vk_device_address(),
            buffers[usize(VertexStreamType::NormalTangent)].vk_device_address(),
            buffers[usize(VertexStreamType::Uv)].vk_device_address(),
            {}
        };

        const u64 item_size = sizeof(shader::StaticMeshData);
        recorder.unbarriered_copy(staging, SubBuffer<BufferUsage::TransferDstBit>(_mesh_datas, item_size, item_size * index));
    }

    recorder.submit_async();

    return mesh_data;
}

void MeshAllocator::recycle(MeshDrawData* data) {
    const auto lock = std::unique_lock(_lock);

    y_debug_assert(data->_mesh_data_index != u32(-1));
    y_debug_assert(_mesh_buffers.size() > data->_mesh_data_index);
    y_debug_assert(!_mesh_buffers[data->_mesh_data_index][0].is_null());

    _should_compact = true;
    _free_blocks << FreeBlock {
        u64(data->_cmd.first_index) / 3,
        u64(data->_cmd.index_count) / 3,
    };

    _free << data->_mesh_data_index;
    _mesh_buffers[data->_mesh_data_index] = {};

    {
        data->_parent = nullptr;
        data->_mesh_data_index = u32(-1);
        data->_mesh_buffers = {};
    }
    y_debug_assert(data->is_null());
}


u64 MeshAllocator::alloc_block(u64 triangle_count) {
    const auto lock = std::unique_lock(_lock);

    sort_and_compact_blocks();

    for(auto& block : _free_blocks) {
        if(block.triangle_count < triangle_count) {
            continue;
        }

        const u64 triangle_offset = block.triangle_offset;

        block.triangle_offset += triangle_count;
        block.triangle_count -= triangle_count;

        return triangle_offset;
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
        return a.triangle_offset < b.triangle_offset;
    });

    usize dst = 0;
    for(usize i = 1; i != block_count; ++i) {
        const u64 dst_end = _free_blocks[dst].triangle_offset + _free_blocks[dst].triangle_count;
        if(_free_blocks[i].triangle_offset == dst_end) {
            y_debug_assert(_free_blocks[dst].triangle_offset + _free_blocks[dst].triangle_count == _free_blocks[i].triangle_offset);
            _free_blocks[dst].triangle_count += _free_blocks[i].triangle_count;
        } else {
            ++dst;
            _free_blocks[dst] = _free_blocks[i];
        }
    }

    _free_blocks.shrink_to(dst + 1);
}


u64 MeshAllocator::available() const {
    const auto lock = std::unique_lock(_lock);
    u64 tris = 0;
    for(const auto& b : _free_blocks) {
        tris += b.triangle_count;
    }
    return tris;
}

u64 MeshAllocator::allocated() const {
    return default_triangle_count - available();
}

usize MeshAllocator::free_blocks() const {
    const auto lock = std::unique_lock(_lock);
    return _free_blocks.size();
}

const TriangleBuffer<>& MeshAllocator::triangle_buffer() const {
    return _triangle_buffer;
}

SubBuffer<BufferUsage::StorageBit> MeshAllocator::mesh_data_buffer() const {
    return _mesh_datas;
}

}
