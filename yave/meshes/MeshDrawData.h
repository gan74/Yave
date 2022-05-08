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
#ifndef YAVE_MESHES_MESH_DRAW_DATA_H
#define YAVE_MESHES_MESH_DRAW_DATA_H

#include "Vertex.h"

#include <yave/graphics/buffers/buffers.h>

#include <memory>

namespace yave {

struct MeshBufferData : NonMovable {
    TriangleSubBuffer triangle_buffer;

    struct AttribBuffers {
        TypedAttribSubBuffer<math::Vec3> positions;
        TypedAttribSubBuffer<math::Vec2ui> normals_tangents;
        TypedAttribSubBuffer<math::Vec2> uvs;
    } attrib_buffers;

    std::array<AttribSubBuffer, 3> untyped_attrib_buffers() const {
        y_debug_assert(attrib_buffers.positions.size() == attrib_buffers.normals_tangents.size());
        y_debug_assert(attrib_buffers.positions.size() == attrib_buffers.uvs.size());

        return std::array<AttribSubBuffer, 3>{
            attrib_buffers.positions,
            attrib_buffers.normals_tangents,
            attrib_buffers.uvs,
        };
    }

    u64 attrib_buffer_elem_count() const {
        return attrib_buffers.positions.size();
    }
};

class MeshDrawData : NonCopyable {
    public:
        MeshDrawData() = default;

        bool is_null() const;

        TriangleSubBuffer triangle_buffer() const;
        const TypedAttribSubBuffer<math::Vec3>& position_buffer() const;

        const MeshBufferData& mesh_buffers() const;

        const VkDrawIndexedIndirectCommand& indirect_data() const;

    private:
        friend class MeshAllocator;

        VkDrawIndexedIndirectCommand _indirect_data = {};

        MeshBufferData* _buffer_data = nullptr;


};

}

#endif // YAVE_MESHES_MESH_DRAW_DATA_H

