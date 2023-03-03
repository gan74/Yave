/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include <y/core/FixedArray.h>
#include <y/core/Chrono.h>

#include <y/math/random.h>

#include <random>

namespace yave {

static core::Vector<PackedVertex> pack_vertices(core::Span<FullVertex> vertices) {
    y_profile();

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

bool MeshData::is_empty() const {
    return _vertices.is_empty() || _triangles.is_empty() || _sub_meshes.is_empty();
}


std::pair<float, core::Vector<Surfel>> MeshData::generate_surfels(usize max) const {
    y_profile();

    float total_area = 0.0f;
    core::FixedArray<float> triangle_cumul_areas(_triangles.size() + 1);
    for(usize i = 0; i != _triangles.size(); ++i) {
        const std::array tri = {
            _vertices[_triangles[i][0]].position,
            _vertices[_triangles[i][1]].position,
            _vertices[_triangles[i][2]].position,
        };

        const float area = triangle_area(tri);
        y_debug_assert(std::isfinite(area));

        triangle_cumul_areas[i] = total_area;
        total_area += area;
    }

    triangle_cumul_areas[_triangles.size()] = total_area;
    y_debug_assert(std::isfinite(total_area));

    const usize count = max;
    const float surfel_area = total_area / count;
    const float min_tri_area = (total_area / count) * 0.001f;

    math::FastRandom rng;
    std::uniform_real_distribution<float> distrib(0.0f, 1.0f);

    auto pts = core::vector_with_capacity<Surfel>(count);
    while(pts.size() < count) {
        const float r = distrib(rng) * total_area;
        const auto* t = std::upper_bound(triangle_cumul_areas.data(), triangle_cumul_areas.data() + triangle_cumul_areas.size(), r);
        const usize index = (t - triangle_cumul_areas.data()) - 1;
        y_debug_assert(index < _triangles.size());

        const float tri_area = triangle_cumul_areas[index + 1] - triangle_cumul_areas[index];
        if(tri_area < min_tri_area) {
            continue;
        }

        const std::array tri = {
            to_surfel(_vertices[_triangles[index][0]]),
            to_surfel(_vertices[_triangles[index][1]]),
            to_surfel(_vertices[_triangles[index][2]]),
        };

        const std::array tri_pos = {tri[0].position, tri[1].position, tri[2].position};

        // https://math.stackexchange.com/questions/538458/how-to-sample-points-on-a-triangle-surface-in-3d
        const float a = distrib(rng);
        const float b = distrib(rng);
        const float sqrt_a = std::sqrt(a);
        const math::Vec3 pos = tri_pos[0] * (1.0f - sqrt_a) + tri_pos[1] * ((1.0f - b) * sqrt_a) + tri_pos[2] * (b * sqrt_a);

        const math::Vec3 bary = barycentric(tri_pos, pos).saturated();
        pts << Surfel {
            pos,
            ((tri[0].normal * bary[0]) + (tri[1].normal * bary[1]) + (tri[2].normal * bary[2])).normalized(),
            ((tri[0].uv * bary[0]) + (tri[1].uv * bary[1]) + (tri[2].uv * bary[2])),
            {},
            surfel_area,
        };
    }

    return {total_area, pts};
}

}

