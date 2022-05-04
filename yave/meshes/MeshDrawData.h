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

namespace yave {



struct MeshDrawData {
#if 1
    TriangleBuffer<> triangle_buffer;
    VertexBuffer<> vertex_buffer;

    struct Streams {
        TypedBuffer<math::Vec3, BufferUsage::AttributeBit | BufferUsage::TransferDstBit, MemoryType::DeviceLocal> positions;
        TypedBuffer<math::Vec2ui, BufferUsage::AttributeBit | BufferUsage::TransferDstBit, MemoryType::DeviceLocal> normals_tangents;
        TypedBuffer<math::Vec2, BufferUsage::AttributeBit | BufferUsage::TransferDstBit, MemoryType::DeviceLocal> uvs;
    } separated_streams;
#else
    TriangleSubBuffer triangle_buffer;
    VertexSubBuffer vertex_buffer;
#endif

    VkDrawIndexedIndirectCommand indirect_data = {};
};

}

#endif // YAVE_MESHES_MESH_DRAW_DATA_H

