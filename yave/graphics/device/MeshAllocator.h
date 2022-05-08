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
#ifndef YAVE_GRAPHICS_DEVICE_MESHALLOCATOR_H
#define YAVE_GRAPHICS_DEVICE_MESHALLOCATOR_H

#include <yave/graphics/buffers/buffers.h>
#include <yave/meshes/MeshDrawData.h>

#include <y/core/Span.h>

#include <atomic>

namespace yave {

class MeshAllocator : NonMovable {

    using MutableTriangleSubBuffer = SubBuffer<BufferUsage::IndexBit | BufferUsage::TransferDstBit>;
    using MutableAttribSubBuffer = SubBuffer<BufferUsage::AttributeBit | BufferUsage::TransferDstBit>;

    public:
        static const u64 default_vertex_count = 16 * 1024 * 1024;
        static const u64 default_triangle_count = 16 * 1024 * 1024;

        MeshAllocator();

        MeshDrawData alloc_mesh(core::Span<PackedVertex> vertices, core::Span<IndexedTriangle> triangles);

    private:
        AttribBuffer<> _attrib_buffer;
        TriangleBuffer<> _triangle_buffer;

        std::atomic<u64> _vertex_offset = 0;
        std::atomic<u64> _triangle_offset = 0;
};

}

#endif // YAVE_GRAPHICS_DEVICE_MESHALLOCATOR_H

