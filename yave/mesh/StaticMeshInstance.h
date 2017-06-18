/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#ifndef YAVE_MESH_STATICMESHINSTANCE_H
#define YAVE_MESH_STATICMESHINSTANCE_H

#include <yave/yave.h>

#include <yave/buffer/TypedBuffer.h>
#include <yave/buffer/TypedSubBuffer.h>

#include "MeshData.h"

namespace yave {

template<MemoryFlags Flags = MemoryFlags::DeviceLocal>
using AttribBuffer = Buffer<BufferUsage::AttributeBit, Flags>;

template<MemoryFlags Flags = MemoryFlags::DeviceLocal>
using TriangleBuffer = TypedBuffer<IndexedTriangle, BufferUsage::IndexBit,Flags>;

template<MemoryFlags Flags = MemoryFlags::DeviceLocal>
using VertexBuffer = TypedBuffer<Vertex, BufferUsage::AttributeBit, Flags>;

template<MemoryFlags Flags = MemoryFlags::DeviceLocal>
using IndirectBuffer = TypedBuffer<vk::DrawIndexedIndirectCommand, BufferUsage::IndirectBit, Flags>;


using AttribSubBuffer = SubBuffer<BufferUsage::AttributeBit>;
using TriangleSubBuffer = TypedSubBuffer<IndexedTriangle, BufferUsage::IndexBit>;
using VertexSubBuffer = TypedSubBuffer<Vertex, BufferUsage::AttributeBit>;
using IndirectSubBuffer = TypedSubBuffer<vk::DrawIndexedIndirectCommand, BufferUsage::IndirectBit>;




class MeshInstancePool;

struct MeshInstanceData {
	TriangleSubBuffer triangle_buffer;
	VertexSubBuffer vertex_buffer;
	vk::DrawIndexedIndirectCommand indirect_data;

	float radius;
};

struct StaticMeshInstance : NonCopyable {

	public:
		StaticMeshInstance() = default;

		~StaticMeshInstance();

		StaticMeshInstance(StaticMeshInstance&& other);
		StaticMeshInstance& operator=(StaticMeshInstance&& other);

		const TriangleSubBuffer& triangle_buffer() const;
		const VertexSubBuffer& vertex_buffer() const;

		const vk::DrawIndexedIndirectCommand& indirect_data() const;

		// indirect data with with buffer offsets baked in, used with CmdBufferRecorder::bind_buffer_no_offset
		vk::DrawIndexedIndirectCommand offset_indirect_data() const;

		float radius() const;

	private:
		friend class MeshInstancePool;

		StaticMeshInstance(MeshInstanceData&& data, MeshInstancePool* pool);

		void swap(StaticMeshInstance& other);

		MeshInstanceData _data;
		MeshInstancePool* _pool = nullptr;
};

}

#endif // YAVE_MESH_STATICMESHINSTANCE_H
