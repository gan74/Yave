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

#include "MeshData.h"
#include <y/io/BuffReader.h>

#include <y/core/Chrono.h>

namespace yave {

float MeshData::radius() const {
	return _radius;
}

const core::Vector<Vertex>& MeshData::vertices() const {
	return _vertices;
}

const core::Vector<IndexedTriangle>& MeshData::triangles() const {
	return _triangles;
}

vk::DrawIndexedIndirectCommand MeshData::indirect_data() const {
	return vk::DrawIndexedIndirectCommand(_triangles.size() * 3, 1);
}

core::Vector<SkinnedVertex> MeshData::skinned_vertices() const {
	if(!_skeleton) {
		fatal("Mesh has no skeleton.");
	}

	auto verts = core::vector_with_capacity<SkinnedVertex>(_vertices.size());
	for(usize i = 0; i != _vertices.size(); ++i) {
		verts << SkinnedVertex{_vertices[i], _skeleton->skin[i]};
	}
	return verts;
}

const core::Vector<Bone>& MeshData::bones() const {
	if(!_skeleton) {
		fatal("Mesh has no skeleton.");
	}
	return _skeleton->bones;
}



static core::Result<Bone> read_bone(io::ReaderRef reader) {
	auto len_res = reader->read_one<u32>();
	if(len_res.is_error()) {
		return core::Err();
	}

	core::String name(nullptr, len_res.unwrap());
	auto name_res = reader->read(name.data(), name.size());
	name[name.size()] = 0;

	auto parent_res = reader->read_one<u32>();

	if(name_res.is_error() || parent_res.is_error()) {
		return core::Err();
	}

	auto transform_res = reader->read_one<BoneTransform>();
	if(transform_res.is_error()) {
		return core::Err();
	}

	return core::Ok(Bone{std::move(name), parent_res.unwrap(), transform_res.unwrap()});
}

MeshData MeshData::from_file(io::ReaderRef reader) {
	const char* err_msg = "Unable to load mesh.";

	struct Header {
		u32 magic;
		u32 type;
		u32 version;

		float radius;

		u32 bones;
		u32 vertices;
		u32 triangles;

		bool is_valid() const {
			return magic == 0x65766179 &&
				   type == 1 &&
				   version == 5 &&
				   vertices != 0 &&
				   triangles != 0;
		}
	};

	core::DebugTimer _("MeshData::from_file()");

	Header header = reader->read_one<Header>().expected(err_msg);
	if(!header.is_valid()) {
		fatal(err_msg);
	}

	MeshData mesh;
	mesh._radius = header.radius;
	mesh._vertices = core::Vector<Vertex>(header.vertices, Vertex{});
	mesh._triangles = core::Vector<IndexedTriangle>(header.triangles, IndexedTriangle{});

	reader->read(mesh._vertices.begin(), header.vertices * sizeof(Vertex)).expected(err_msg);
	reader->read(mesh._triangles.begin(), header.triangles * sizeof(IndexedTriangle)).expected(err_msg);

	if(header.bones) {
		mesh._skeleton = new SkeletonData();
		mesh._skeleton->skin = core::Vector<SkinWeights>(header.vertices, SkinWeights{});
		reader->read(mesh._skeleton->skin.begin(), header.vertices * sizeof(SkinWeights)).expected(err_msg);

		for(usize i = 0; i != header.bones; ++i) {
			mesh._skeleton->bones << read_bone(reader).expected(err_msg);
		}
	}

	return mesh;
}



}
