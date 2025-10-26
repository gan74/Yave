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
#include <yave/graphics/descriptors/DescriptorArray.h>
#include <yave/meshes/MeshDrawData.h>

#include <yave/graphics/shader_structs.h>

#include <y/core/Span.h>
#include <y/core/Vector.h>

#include <atomic>
#include <mutex>

namespace yave {

class MeshBufferArray final : public DescriptorArray {
    public:
        static constexpr usize stream_count = usize(VertexStreamType::Max);

        using Indices = std::array<u32, stream_count>;

        MeshBufferArray();

        Indices create_buffers(const MeshVertexStreams& streams, TransferCmdBufferRecorder& recorder);
        void remove_buffers(Indices indices);

    private:
        using DataBuffer = Buffer<BufferUsage::StorageBit | BufferUsage::TransferDstBit, MemoryType::DeviceLocal>;

        core::Vector<DataBuffer> _buffers;
};

class MeshAllocator : NonMovable {
    using MutableTriangleSubBuffer = SubBuffer<BufferUsage::IndexBit | BufferUsage::TransferDstBit>;
    using MutableAttribSubBuffer = SubBuffer<BufferUsage::AttributeBit | BufferUsage::TransferDstBit>;

    template<typename T>
    using TypedDataBuffer = TypedBuffer<T, BufferUsage::StorageBit | BufferUsage::TransferDstBit | BufferUsage::TransferSrcBit, MemoryType::DeviceLocal>;

    struct FreeBlock {
        u64 vertex_offset;
        u64 vertex_count;

        u64 triangle_offset;
        u64 triangle_count;
    };

    public:
        static const u64 default_vertex_count = 8 * 1024 * 1024;
        static const u64 default_triangle_count = 8 * 1024 * 1024;

        MeshAllocator();
        ~MeshAllocator();

        MeshDrawData alloc_mesh(const MeshVertexStreams& streams, core::Span<IndexedTriangle> triangles);

        std::pair<u64, u64> available() const; // slow!
        std::pair<u64, u64> allocated() const; // slow!
        usize free_blocks() const;

        const MeshDrawBuffers& mesh_buffers() const;

        const MeshBufferArray& mesh_buffer_array() const;
        SubBuffer<BufferUsage::StorageBit> mesh_data_buffer() const;

    private:
        friend class MeshDrawData;

        std::pair<u64, u64> alloc_block(u64 vertex_count, u64 triangle_count);
        void sort_and_compact_blocks();

        void recycle(MeshDrawData* data);

        AttribBuffer<> _attrib_buffer;
        TriangleBuffer<> _triangle_buffer;

        core::Vector<FreeBlock> _free_blocks;
        bool _should_compact = false;
        mutable ProfiledLock<> _lock;

        std::unique_ptr<MeshDrawBuffers> _mesh_buffers;

        core::Vector<u32> _free;
        TypedDataBuffer<shader::StaticMeshData> _mesh_datas;
        MeshBufferArray _mesh_buffer_array;
};

}

#endif // YAVE_GRAPHICS_DEVICE_MESHALLOCATOR_H

