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

#include "MeshInstancePool.h"

#include <numeric>
#include <functional>

namespace yave {

MeshInstancePool::MeshInstancePool(DevicePtr dptr, usize vertices, usize triangles) :
		DeviceLinked(dptr),
		_vertex_buffer(dptr, vertices),
		_vertices_blocks{FreeBlock{0, _vertex_buffer.size()}},
		_triangle_buffer(dptr, triangles),
		_triangles_blocks{FreeBlock{0, _triangle_buffer.size()}} {
}



usize MeshInstancePool::free_vertices() const {
	return std::accumulate(_vertices_blocks.begin(), _vertices_blocks.end(), 0, [](usize t, const FreeBlock& b) { return t + b.size; });
}

usize MeshInstancePool::free_triangles() const {
	return std::accumulate(_triangles_blocks.begin(), _triangles_blocks.end(), 0, [](usize t, const FreeBlock& b) { return t + b.size; });
}

core::Result<StaticMeshInstance, MeshInstancePool::Error> MeshInstancePool::create_static_mesh(const MeshData& data) {
	auto vertex_block = alloc_block(_vertices_blocks, data.vertices.size()).expected("Unable to allocate vertex sub-buffer.");
	auto triangle_block = alloc_block(_triangles_blocks, data.triangles.size()).expected("Unable to allocate triangle sub-buffer.");

	VertexSubBuffer verts(_vertex_buffer, vertex_block.offset, vertex_block.size);
	TriangleSubBuffer tris(_triangle_buffer, triangle_block.offset, triangle_block.size);

	{
		auto triangle_mapping = tris.map();
		auto vertex_mapping = verts.map();
		std::copy(data.triangles.begin(), data.triangles.end(), triangle_mapping.begin());
		std::copy(data.vertices.begin(), data.vertices.end(), vertex_mapping.begin());
	}

	vk::DrawIndexedIndirectCommand cmd(data.triangles.size() * 3, 1);
	return core::Ok(StaticMeshInstance(MeshInstanceData{tris, verts, cmd, data.radius}, this));
}

core::Result<MeshInstancePool::FreeBlock> MeshInstancePool::alloc_block(core::Vector<FreeBlock>& blocks, usize size) {
	for(auto it = blocks.begin(); it != blocks.end(); ++it) {
		if(it->size == size) {
			FreeBlock alloc = *it;
			blocks.erase_unordered(it);
			return core::Ok(alloc);
		} else if(it->size > size) {
			FreeBlock alloc{it->offset, size};
			it->offset += size;
			it->size -= size;
			return core::Ok(alloc);
		}
	}

	return core::Err();
}

void MeshInstancePool::free_block(core::Vector<FreeBlock>& blocks, FreeBlock block) {
	usize end = block.offset + block.size;
	for(auto& bl : blocks) {
		if(bl.offset + bl.size == block.offset) {
			bl.size += block.size;
			return;
		} else if(end == bl.offset) {
			bl.size += block.size;
			bl.offset = block.offset;
			return;
		}
	}

	blocks << block;
}

void MeshInstancePool::release(MeshInstanceData&& data) {
	if(data.triangle_buffer.vk_buffer() != _triangle_buffer.vk_buffer() ||
	   data.vertex_buffer.vk_buffer() != _vertex_buffer.vk_buffer()) {
		fatal("MeshInstanceData was not returned to its original pool.");
	}

	free_block(_vertices_blocks, FreeBlock{data.vertex_buffer.offset(), data.vertex_buffer.size()});
	free_block(_triangles_blocks, FreeBlock{data.triangle_buffer.offset(), data.triangle_buffer.size()});
}

}
