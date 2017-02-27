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
using TriangleSubBuffer = TypedSubBuffer<IndexedTriangle, BufferUsage::IndexBit,Flags>;

template<MemoryFlags Flags = MemoryFlags::DeviceLocal>
using VertexSubBuffer = TypedSubBuffer<Vertex, BufferUsage::AttributeBit, Flags>;

template<MemoryFlags Flags = MemoryFlags::DeviceLocal>
using IndirectSubBuffer = TypedSubBuffer<vk::DrawIndexedIndirectCommand, BufferUsage::IndirectBit, Flags>;



inline auto create_indirect_buffer_data(const MeshData& m, usize vertex_offset, usize triangle_offset) {
	using Cmd = vk::DrawIndexedIndirectCommand;
	/*core::Vector<Cmd> cmds;
	for(usize i = 0, size = m.triangles.size(); i < size; i += max) {
		cmds << Cmd(u32(std::min(size - i, max) * 3), 1, u32((i + triangle_offset) * 3), i32(vertex_offset));
	}
	return cmds;*/
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

		/*StaticMeshInstance(const TriangleSubBuffer<>& t, const VertexSubBuffer<>& v, const IndirectSubBuffer<>&& i, float rad) :
				triangle_buffer(t),
				vertex_buffer(v),
				indirect_buffer(i),
				radius(rad) {
		}

		StaticMeshInstance(const TriangleSubBuffer<>& t, const VertexSubBuffer<>& v, IndirectBuffer<>&& i, float rad) :
				StaticMeshInstance(t, v, IndirectSubBuffer<>(i), rad) {

			_keep_alive.indirect = std::move(i);
		}

		StaticMeshInstance(TriangleBuffer<>&& t, VertexBuffer<>&& v, IndirectBuffer<>&& i, float rad) :
				StaticMeshInstance(TriangleSubBuffer<>(t), VertexSubBuffer<>(v), IndirectSubBuffer<>(i), rad) {

			_keep_alive = { std::move(t), std::move(v), std::move(i) };
		}

		StaticMeshInstance(DevicePtr dptr, const MeshData& data) :
				 StaticMeshInstance(TriangleBuffer<>(dptr, data.triangles),
									VertexBuffer<>(dptr, data.vertices),
									IndirectBuffer<>(dptr, create_indirect_buffer_data(data)),
									data.radius) {

		}*/

		const TriangleSubBuffer<> triangle_buffer;
		const VertexSubBuffer<> vertex_buffer;
		vk::DrawIndexedIndirectCommand indirect_data;
		//const IndirectSubBuffer<> indirect_buffer;


		const float radius = 0.0f;

	private:
		/*struct {
			TriangleBuffer<> triangle;
			VertexBuffer<> vertex;
			IndirectBuffer<> indirect;
		} _keep_alive;*/

};

}

#endif // YAVE_MESH_STATICMESHINSTANCE_H
