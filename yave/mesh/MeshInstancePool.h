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
#ifndef YAVE_MESH_MESHINSTANCEPOOL_H
#define YAVE_MESH_MESHINSTANCEPOOL_H

#include <yave/yave.h>

#include "StaticMeshInstance.h"

#include <map>

namespace yave {

class MeshInstancePool : NonCopyable, public DeviceLinked {

	struct FreeBlock {
		usize offset;
		usize size;
	};

	public:
		enum class Error {
			TriangleBufferFull,
			VertexBufferFull
		};

		MeshInstancePool(DevicePtr dptr, usize vertices = 1024 * 1024, usize triangles = 1024 * 1024);

		core::Result<StaticMeshInstance, Error> create_static_mesh(const MeshData& data);

		usize free_vertices() const;
		usize free_triangles() const;

	private:
		friend class StaticMeshInstance;

		static core::Result<FreeBlock> alloc_block(core::Vector<FreeBlock>& blocks, usize size);
		static void free_block(core::Vector<FreeBlock>& blocks, FreeBlock block);

		void release(MeshInstanceData&& data);


		VertexBuffer<> _vertex_buffer;
		core::Vector<FreeBlock> _vertices_blocks;

		TriangleBuffer<> _triangle_buffer;
		core::Vector<FreeBlock> _triangles_blocks;
};

}

#endif // YAVE_MESH_MESHINSTANCEPOOL_H
