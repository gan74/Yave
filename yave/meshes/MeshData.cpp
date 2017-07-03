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


MeshData MeshData::from_parts(core::Vector<Vertex>&& vertices, core::Vector<IndexedTriangle>&& triangles, core::Vector<SkinWeights>&& skin, core::Vector<Bone>&& bones) {
	if(bones.is_empty() != skin.is_empty()) {
		fatal("Invalid skeleton.");
	}
	if((!skin.is_empty() && skin.size() != vertices.size())) {
		fatal("Invalid skin data.");
	}

	float radius = 0.0f;
	std::for_each(vertices.begin(), vertices.end(), [&](const auto& v) { radius = std::max(radius, v.position.length2()); });

	MeshData mesh;
	mesh._vertices = std::move(vertices);
	mesh._triangles = std::move(triangles);
	mesh._radius = std::sqrt(radius);

	if(!skin.is_empty()) {
		mesh._skeleton = new SkeletonData{std::move(skin), std::move(bones)};
	}

	return mesh;
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
			u32 name_len = reader->read_one<u32>().expected(err_msg);
			core::String name(nullptr, name_len);
			reader->read(name.data(), name.size()).expected(err_msg);
			name[name.size()] = 0;

			u32 parent = reader->read_one<u32>().expected(err_msg);
			BoneTransform transform = reader->read_one<BoneTransform>().expected(err_msg);

			mesh._skeleton->bones << Bone{std::move(name), parent, transform};
		}
	}

	return mesh;
}

void MeshData::to_file(io::WriterRef writer) const {
	const char* err_msg = "Unable to write mesh.";

	u32 magic = 0x65766179;
	u32 type = 1;
	u32 version = 5;

	float radius = _radius;

	u32 bones = _skeleton ? _skeleton->bones.size() : 0;
	u32 vertices = _vertices.size();
	u32 triangles = _triangles.size();

	writer->write_one(magic).expected(err_msg);
	writer->write_one(type).expected(err_msg);
	writer->write_one(version).expected(err_msg);

	writer->write_one(radius).expected(err_msg);

	writer->write_one(bones).expected(err_msg);
	writer->write_one(vertices).expected(err_msg);
	writer->write_one(triangles).expected(err_msg);

	writer->write(_vertices.begin(), _vertices.size() * sizeof(Vertex)).expected(err_msg);
	writer->write(_triangles.begin(), _triangles.size() * sizeof(IndexedTriangle)).expected(err_msg);


	if(_skeleton) {
		writer->write(_skeleton->skin.begin(), _skeleton->skin.size() * sizeof(SkinWeights)).expected(err_msg);

		for(const auto& bone : _skeleton->bones) {
			writer->write_one(u32(bone.name.size())).expected(err_msg);
			writer->write(bone.name.data(), bone.name.size()).expected(err_msg);

			writer->write_one(bone.parent).expected(err_msg);
			writer->write_one(bone.local_transform).expected(err_msg);
		}
	}
}



}
