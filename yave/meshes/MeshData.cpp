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

#include <y/core/Chrono.h>

namespace yave {

MeshData::MeshData(core::Vector<Vertex>&& vertices, core::Vector<IndexedTriangle>&& triangles, core::Vector<SkinWeights>&& skin, core::Vector<Bone>&& bones) :
		_vertices(std::move(vertices)),
		_triangles(std::move(triangles)) {

	if(bones.is_empty() != skin.is_empty()) {
		y_fatal("Invalid skeleton.");
	}
	if((!skin.is_empty() && skin.size() != _vertices.size())) {
		y_fatal("Invalid skin data.");
	}

	math::Vec3 max(-std::numeric_limits<float>::max());
	math::Vec3 min(std::numeric_limits<float>::max());
	std::for_each(_vertices.begin(), _vertices.end(), [&](const Vertex& v) {
		max = max.max(v.position);
		min = min.min(v.position);
	});
	_aabb = AABB(min, max);

	if(!skin.is_empty()) {
		_skeleton = std::make_unique<SkeletonData>(SkeletonData{std::move(skin), std::move(bones)});
	}
}

float MeshData::radius() const {
	return _aabb.origin_radius();
}

const AABB& MeshData::aabb() const {
	return _aabb;
}

core::ArrayView<Vertex> MeshData::vertices() const {
	return _vertices;
}

core::ArrayView<IndexedTriangle> MeshData::triangles() const {
	return _triangles;
}

core::ArrayView<Bone> MeshData::bones() const {
	if(!_skeleton) {
		y_fatal("Mesh has no skeleton.");
	}
	return _skeleton->bones;
}

core::ArrayView<SkinWeights> MeshData::skin() const {
	if(!_skeleton) {
		y_fatal("Mesh has no skeleton.");
	}
	return _skeleton->skin;
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

}
