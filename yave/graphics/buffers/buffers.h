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
#ifndef YAVE_GRAPHICS_BUFFERS_BUFFERS_H
#define YAVE_GRAPHICS_BUFFERS_BUFFERS_H

#include <yave/meshes/Vertex.h>
#include <yave/graphics/memory/MemoryType.h>

#include "BufferUsage.h"

namespace yave {

template<BufferUsage Usage = BufferUsage::None, MemoryType Memory = MemoryType::DontCare>
class SubBuffer;

template<BufferUsage Usage, MemoryType Memory = prefered_memory_type(Usage)>
class Buffer;

template<typename Elem, typename Buff>
class TypedWrapper;

template<typename Elem, BufferUsage Usage, MemoryType Memory = prefered_memory_type(Usage)>
using TypedBuffer = TypedWrapper<Elem, Buffer<Usage, Memory>>;

template<typename Elem, BufferUsage Usage = BufferUsage::None, MemoryType Memory = MemoryType::DontCare>
using TypedSubBuffer = TypedWrapper<Elem, SubBuffer<Usage, Memory>>;






using StagingBuffer = Buffer<BufferUsage::TransferSrcBit, MemoryType::Staging>;
using StagingSubBuffer = SubBuffer<BufferUsage::TransferSrcBit, MemoryType::Staging>;

template<typename T>
using TypedStagingBuffer = TypedBuffer<T, BufferUsage::TransferSrcBit, MemoryType::Staging>;



template<MemoryType Memory = prefered_memory_type(BufferUsage::AttributeBit)>
using AttribBuffer = Buffer<BufferUsage::AttributeBit | BufferUsage::TransferDstBit, Memory>;

template<typename T, MemoryType Memory = prefered_memory_type(BufferUsage::AttributeBit)>
using TypedAttribBuffer = TypedBuffer<T, BufferUsage::AttributeBit, Memory>;



template<MemoryType Memory = prefered_memory_type(BufferUsage::UniformBit)>
using UniformBuffer = Buffer<BufferUsage::UniformBit, Memory>;

template<typename T, MemoryType Memory = prefered_memory_type(BufferUsage::UniformBit)>
using TypedUniformBuffer = TypedBuffer<T, BufferUsage::UniformBit, Memory>;



template<MemoryType Memory = prefered_memory_type(BufferUsage::IndexBit)>
using TriangleBuffer = TypedBuffer<IndexedTriangle, BufferUsage::IndexBit | BufferUsage::TransferDstBit, Memory>;

template<MemoryType Memory = prefered_memory_type(BufferUsage::AttributeBit)>
using VertexBuffer = TypedBuffer<PackedVertex, BufferUsage::AttributeBit | BufferUsage::TransferDstBit, Memory>;



using TriangleSubBuffer = TypedSubBuffer<IndexedTriangle, BufferUsage::IndexBit>;
using VertexSubBuffer = TypedSubBuffer<PackedVertex, BufferUsage::AttributeBit>;

using AttribSubBuffer = SubBuffer<BufferUsage::AttributeBit>;
using IndexSubBuffer = SubBuffer<BufferUsage::IndexBit>;

using IndirectSubBuffer = TypedSubBuffer<VkDrawIndexedIndirectCommand, BufferUsage::IndirectBit>;

template<typename T>
using TypedAttribSubBuffer = TypedSubBuffer<T, BufferUsage::AttributeBit>;


template<typename T>
using TypedReadBackBuffer = TypedBuffer<T, BufferUsage::StorageBit, MemoryType::CpuVisible>;

}

#endif // YAVE_GRAPHICS_BUFFERS_BUFFERS_H

