/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#ifndef YAVE_MESHES_STATICMESH_H
#define YAVE_MESHES_STATICMESH_H

#include "MeshData.h"
#include <yave/buffers/buffers.h>
#include <yave/buffers/TypedMapping.h>

namespace yave {

class StaticMesh : NonCopyable {

	public:
		using load_from = MeshData;

		StaticMesh() = default;

		StaticMesh(DevicePtr dptr, const MeshData& mesh_data);

		StaticMesh(StaticMesh&& other);
		StaticMesh& operator=(StaticMesh&& other);

		const TriangleBuffer<>& triangle_buffer() const;
		const VertexBuffer<>& vertex_buffer() const;
		const vk::DrawIndexedIndirectCommand& indirect_data() const;

		float radius() const;

	private:
		void swap(StaticMesh& other);

		TriangleBuffer<> _triangle_buffer;
		VertexBuffer<> _vertex_buffer;
		vk::DrawIndexedIndirectCommand _indirect_data;

		float _radius;
};

}

#endif // YAVE_MESHES_STATICMESH_H
