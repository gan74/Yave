/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_BUFFERS_BUFFERS_H
#define YAVE_GRAPHICS_BUFFERS_BUFFERS_H

#include <yave/meshes/Vertex.h>

#include "TypedWrapper.h"

namespace yave {

using StagingBuffer = Buffer<BufferUsage::TransferSrcBit, MemoryType::Staging>;

template<typename T>
using TypedStagingBuffer = TypedBuffer<T, StagingBuffer::usage, StagingBuffer::memory_type>;


template<MemoryType Memory = prefered_memory_type(BufferUsage::AttributeBit)>
using AttribBuffer = Buffer<BufferUsage::AttributeBit, Memory>;

template<typename T, MemoryType Memory = prefered_memory_type(BufferUsage::AttributeBit)>
using TypedAttribBuffer = TypedBuffer<T, BufferUsage::AttributeBit, Memory>;


template<MemoryType Memory = prefered_memory_type(BufferUsage::UniformBit)>
using UniformBuffer = Buffer<BufferUsage::UniformBit, Memory>;

template<typename T, MemoryType Memory = prefered_memory_type(BufferUsage::UniformBit)>
using TypedUniformBuffer = TypedBuffer<T, BufferUsage::UniformBit, Memory>;



template<MemoryType Memory = prefered_memory_type(BufferUsage::IndexBit)>
using TriangleBuffer = TypedBuffer<IndexedTriangle, BufferUsage::IndexBit | BufferUsage::TransferDstBit, Memory>;

template<MemoryType Memory = prefered_memory_type(BufferUsage::AttributeBit)>
using VertexBuffer = TypedBuffer<Vertex, BufferUsage::AttributeBit | BufferUsage::TransferDstBit, Memory>;

template<MemoryType Memory = prefered_memory_type(BufferUsage::AttributeBit)>
using SkinnedVertexBuffer = TypedBuffer<SkinnedVertex, BufferUsage::AttributeBit | BufferUsage::TransferDstBit, Memory>;

template<MemoryType Memory = prefered_memory_type(BufferUsage::IndirectBit)>
using IndirectBuffer = TypedBuffer<VkDrawIndexedIndirectCommand, BufferUsage::IndirectBit | BufferUsage::TransferDstBit, Memory>;


template<typename T>
using AttribSubBuffer = TypedSubBuffer<T, BufferUsage::AttributeBit>;

using TriangleSubBuffer = TypedSubBuffer<IndexedTriangle, BufferUsage::IndexBit>;
using VertexSubBuffer = TypedSubBuffer<Vertex, BufferUsage::AttributeBit>;
using SkinnedVertexSubBuffer = TypedSubBuffer<SkinnedVertex, BufferUsage::AttributeBit>;
using IndirectSubBuffer = TypedSubBuffer<VkDrawIndexedIndirectCommand, BufferUsage::IndirectBit>;

}

#endif // YAVE_GRAPHICS_BUFFERS_BUFFERS_H

