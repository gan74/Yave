/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef YAVE_MESHES_MESHDRAWDATA_H
#define YAVE_MESHES_MESHDRAWDATA_H

#include "MeshVertexStreams.h"

#include <yave/graphics/raytracing/AccelerationStructure.h>

#include <memory>

namespace yave {

struct MeshDrawCommand {
    u32 index_count = 0;
    u32 first_index = 0;
    i32 vertex_offset = 0;

    VkDrawIndexedIndirectCommand vk_indirect_data(u32 instance_index = 0, u32 instance_count = 1) const {
        return VkDrawIndexedIndirectCommand {
            index_count,
            instance_count,
            first_index,
            vertex_offset,
            instance_index
        };
    }
};

class MeshDrawBuffers : NonMovable {
    public:
        static constexpr usize vertex_stream_count = MeshVertexStreams::stream_count;

        MeshDrawBuffers() = default;

        core::Span<AttribSubBuffer> attrib_buffers() const;
        TriangleSubBuffer triangle_buffer() const;

        usize vertex_count() const;

        MeshAllocator* parent() const;

    private:
        friend class MeshAllocator;

        usize _vertex_count = 0;
        std::array<AttribSubBuffer, vertex_stream_count> _attrib_buffers;
        TriangleSubBuffer _triangle_buffer;

        MeshAllocator* _parent = nullptr;
};

class MeshDrawData : NonCopyable {
    public:
        MeshDrawData() = default;
        MeshDrawData(MeshDrawData&& other);
        MeshDrawData& operator=(MeshDrawData&& other);

        ~MeshDrawData();

        bool is_null() const;

        const MeshDrawBuffers& mesh_buffers() const;
        TriangleSubBuffer triangle_buffer() const;

        const MeshDrawCommand& draw_command() const;

        const BLAS& blas() const;

    private:
        friend class LifetimeManager;
        friend class MeshAllocator;

        void recycle();

    private:
        void swap(MeshDrawData& other);

        MeshDrawCommand _command = {};
        u32 _vertex_count = 0;

        MeshDrawBuffers* _mesh_buffers = nullptr;

        BLAS _blas;
};

}

#endif // YAVE_MESHES_MESHDRAWDATA_H

