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
using TriangleBuffer = TypedBuffer<IndexedTriangle, BufferUsage::IndexBit,Flags>;

template<MemoryFlags Flags = MemoryFlags::DeviceLocal>
using VertexBuffer = TypedBuffer<Vertex, BufferUsage::AttributeBit, Flags>;

template<MemoryFlags Flags = MemoryFlags::DeviceLocal>
using IndirectBuffer = TypedBuffer<vk::DrawIndexedIndirectCommand, BufferUsage::IndirectBit, Flags>;



template<MemoryFlags Flags = MemoryFlags::DeviceLocal>
using TriangleSubBuffer = TypedSubBuffer<IndexedTriangle, BufferUsage::IndexBit, Flags>;

template<MemoryFlags Flags = MemoryFlags::DeviceLocal>
using VertexSubBuffer = TypedSubBuffer<Vertex, BufferUsage::AttributeBit, Flags>;

template<MemoryFlags Flags = MemoryFlags::DeviceLocal>
using IndirectSubBuffer = TypedSubBuffer<vk::DrawIndexedIndirectCommand, BufferUsage::IndirectBit, Flags>;



inline auto create_indirect_buffer_data(const MeshData& m, usize vertex_offset, usize triangle_offset) {
	using Cmd = vk::DrawIndexedIndirectCommand;
	return Cmd(m.triangles.size() * 3, 1, triangle_offset * 3, vertex_offset);
}

struct StaticMeshInstance {

	public:
		StaticMeshInstance(const TriangleSubBuffer<>& t, const VertexSubBuffer<>& v, const vk::DrawIndexedIndirectCommand& i, float r) :
				triangle_buffer(t),
				vertex_buffer(v),
				indirect_data(i),
				radius(r) {
		}



		const TriangleSubBuffer<> triangle_buffer;
		const VertexSubBuffer<> vertex_buffer;
		vk::DrawIndexedIndirectCommand indirect_data;

		const float radius = 0.0f;

	private:
};

}

#endif // YAVE_MESH_STATICMESHINSTANCE_H
