/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

static core::Vector<PackedVertex> pack_vertices(core::Span<FullVertex> vertices) {
    auto packed = core::vector_with_capacity<PackedVertex>(vertices.size());
    for(const FullVertex& v : vertices) {
        packed << pack_vertex(v);
    }
    return packed;
}

MeshData::MeshData(core::Span<FullVertex> vertices, core::Span<IndexedTriangle> triangles) {
    add_sub_mesh(vertices, triangles);
}

MeshData::MeshData(core::Span<PackedVertex> vertices, core::Span<IndexedTriangle> triangles) {
    add_sub_mesh(vertices, triangles);
}

void MeshData::add_sub_mesh(core::Span<FullVertex> vertices, core::Span<IndexedTriangle> triangles) {
    add_sub_mesh(pack_vertices(vertices), triangles);
}

void MeshData::add_sub_mesh(core::Span<PackedVertex> vertices, core::Span<IndexedTriangle> triangles) {
    y_debug_assert(!vertices.is_empty());
    y_debug_assert(!triangles.is_empty());

    const u32 vertex_offset = u32(_vertices.size());
    const u32 first_triangle = u32(_triangles.size());

    _vertices.set_min_capacity(_vertices.size() + vertices.size());
    _triangles.set_min_capacity(_triangles.size() + triangles.size());

    std::copy(vertices.begin(), vertices.end(), std::back_inserter(_vertices));
    std::transform(triangles.begin(), triangles.end(), std::back_inserter(_triangles), [=](IndexedTriangle tri) {
        return IndexedTriangle {
            tri[0] + vertex_offset,
            tri[1] + vertex_offset,
            tri[2] + vertex_offset,
        };
    });

    {
        math::Vec3 max(-std::numeric_limits<float>::max());
        math::Vec3 min(std::numeric_limits<float>::max());
        std::for_each(vertices.begin(), vertices.end(), [&](const PackedVertex& v) {
            max = max.max(v.position);
            min = min.min(v.position);
        });

        _aabb = _sub_meshes.is_empty() ? AABB(min, max) : _aabb.merged(AABB(min, max));

        y_debug_assert(math::fully_finite(_aabb.min()));
        y_debug_assert(math::fully_finite(_aabb.max()));
    }

    _sub_meshes << SubMesh{u32(triangles.size()), first_triangle};
}

float MeshData::radius() const {
    return _aabb.origin_radius();
}

const AABB& MeshData::aabb() const {
    return _aabb;
}

core::Span<PackedVertex> MeshData::vertices() const {
    return _vertices;
}

core::Span<IndexedTriangle> MeshData::triangles() const {
    return _triangles;
}

core::Span<MeshData::SubMesh> MeshData::sub_meshes() const {
    return _sub_meshes;
}

core::Span<Bone> MeshData::bones() const {
    if(!_skeleton) {
        return {};
    }
    return _skeleton->bones;
}

core::Span<SkinWeights> MeshData::skin() const {
    if(!_skeleton) {
        return {};
    }
    return _skeleton->skin;
}

core::Vector<SkinnedVertex> MeshData::skinned_vertices() const {
    if(!_skeleton) {
        return {};
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

