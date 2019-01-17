/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

const core::Vector<SkinWeights> MeshData::skin() const {
	if(!_skeleton) {
		y_fatal("Mesh has no skeleton.");
	}
	return _skeleton->skin;
}

const core::Vector<Bone>& MeshData::bones() const {
	if(!_skeleton) {
		y_fatal("Mesh has no skeleton.");
	}
	return _skeleton->bones;
}

core::Vector<SkinnedVertex> MeshData::skinned_vertices() const {
	if(!_skeleton) {
		y_fatal("Mesh has no skeleton.");
	}

	auto verts = core::vector_with_capacity<SkinnedVertex>(_vertices.size());
	for(usize i = 0; i != _vertices.size(); ++i) {
		verts << SkinnedVertex{_vertices[i], _skeleton->skin[i]};
	}
	return verts;
}

bool MeshData::has_skeleton() const {
	return bool(_skeleton);
}

MeshData MeshData::from_parts(core::Vector<Vertex>&& vertices, core::Vector<IndexedTriangle>&& triangles, core::Vector<SkinWeights>&& skin, core::Vector<Bone>&& bones) {
	if(bones.is_empty() != skin.is_empty()) {
		y_fatal("Invalid skeleton.");
	}
	if((!skin.is_empty() && skin.size() != vertices.size())) {
		y_fatal("Invalid skin data.");
	}

	float radius = 0.0f;
	std::for_each(vertices.begin(), vertices.end(), [&](const auto& v) { radius = std::max(radius, v.position.length2()); });

	MeshData mesh;
	mesh._vertices = std::move(vertices);
	mesh._triangles = std::move(triangles);
	mesh._radius = std::sqrt(radius);

	if(!skin.is_empty()) {
		mesh._skeleton = std::make_unique<SkeletonData>(SkeletonData{std::move(skin), std::move(bones)});
	}

	return mesh;
}

}
