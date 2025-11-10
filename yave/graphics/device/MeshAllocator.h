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
#ifndef YAVE_GRAPHICS_DEVICE_MESHALLOCATOR_H
#define YAVE_GRAPHICS_DEVICE_MESHALLOCATOR_H

#include <yave/graphics/buffers/Buffer.h>
#include <yave/meshes/MeshDrawData.h>

#include <yave/graphics/shader_structs.h>

#include <y/core/Span.h>
#include <y/core/Vector.h>

#include <mutex>

namespace yave {

class MeshAllocator : NonMovable {
    using MutableTriangleSubBuffer = SubBuffer<BufferUsage::IndexBit | BufferUsage::TransferDstBit>;

    using VertexBuffer = Buffer<BufferUsage::StorageBit | BufferUsage::AccelStructureInputBit | BufferUsage::TransferDstBit, MemoryType::DeviceLocal>;

    static constexpr usize stream_count = usize(VertexStreamType::Max);
    using Buffers = std::array<VertexBuffer, stream_count>;

    template<typename T>
    using TypedDataBuffer = TypedBuffer<T, BufferUsage::StorageBit | BufferUsage::TransferDstBit | BufferUsage::TransferSrcBit, MemoryType::DeviceLocal>;

    struct FreeBlock {
        u64 triangle_offset;
        u64 triangle_count;
    };

    public:
        static const u64 default_triangle_count = 8 * 1024 * 1024;

        MeshAllocator();
        ~MeshAllocator();

        MeshDrawData alloc_mesh(const MeshVertexStreams& streams, core::Span<IndexedTriangle> triangles);

        SubBuffer<BufferUsage::StorageBit> mesh_data_buffer() const;

        u64 available() const; // slow!
        u64 allocated() const; // slow!
        usize free_blocks() const;

        const TriangleBuffer<>& triangle_buffer() const;

    private:
        friend class MeshDrawData;

        void recycle(MeshDrawData* data);
        u64 alloc_block(u64 triangle_count);
        void sort_and_compact_blocks();

        TriangleBuffer<> _triangle_buffer;

        core::Vector<FreeBlock> _free_blocks;
        bool _should_compact = false;
        mutable ProfiledLock<> _lock;

        core::Vector<u32> _free;
        core::Vector<Buffers> _mesh_buffers;
        TypedDataBuffer<shader::StaticMeshData> _mesh_datas;
};

}

#endif // YAVE_GRAPHICS_DEVICE_MESHALLOCATOR_H

