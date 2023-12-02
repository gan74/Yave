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
#include "Meshlet.h"

#include <y/core/HashMap.h>
#include <y/core/Chrono.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <mutex>

namespace yave {

static core::Vector<PackedVertex> pack_vertices(core::Span<FullVertex> vertices) {
    auto packed = core::Vector<PackedVertex>::with_capacity(vertices.size());
    for(const FullVertex& v : vertices) {
        packed << pack_vertex(v);
    }
    return packed;
}

void MeshData::meshletify() const {
    usize i = 0;
    usize m = 0;

    core::Vector<IndexedTriangle> sorted(_triangles);
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a[0] < b[0]; });

    while(i != _triangles.size()) {
        core::FlatHashMap<u32, u8> vertex_mapping;
        usize tris = 0;
        while(tris < Meshlet::max_triangles && i != _triangles.size()) {
            const IndexedTriangle tri = _triangles[i];

            bool inserted = true;
            for(usize k = 0; k != 3; ++k) {
                if(vertex_mapping.size() == Meshlet::max_vertices) {
                    inserted = false;
                    break;
                }
                if(!vertex_mapping.contains(tri[k])) {
                    vertex_mapping[tri[k]] = u8(vertex_mapping.size());
                }
            }

            if(inserted) {
                ++tris;
                ++i;
            } else {
                tris = Meshlet::max_triangles;
            }
        }
        ++m;
    }

    y_debug_assert(i == _triangles.size());
    const usize min_meshlets_v = vertex_streams().vertex_count() / Meshlet::max_vertices + (vertex_streams().vertex_count() % Meshlet::max_vertices ? 1 : 0);
    const usize min_meshlets_t = _triangles.size() / Meshlet::max_triangles + (_triangles.size() % Meshlet::max_triangles ? 1 : 0);

    static std::mutex lock;

    const auto l = y_profile_unique_lock(lock);
    log_msg(fmt("% meshlets (% min)", m, std::min(min_meshlets_v, min_meshlets_t)));
    log_msg(fmt("    % triangles -> % triangles", i, m * Meshlet::max_triangles));
    log_msg(fmt("    % vertices -> % vertices", vertex_streams().vertex_count(), m * Meshlet::max_vertices));
}

MeshData::MeshData(core::Span<FullVertex> vertices, core::Span<IndexedTriangle> triangles) {
    add_sub_mesh(vertices, triangles);
}

MeshData::MeshData(core::Span<PackedVertex> vertices, core::Span<IndexedTriangle> triangles) {
    add_sub_mesh(vertices, triangles);
}

MeshData::MeshData(MeshVertexStreams streams, core::Span<IndexedTriangle> triangles) {
    add_sub_mesh(streams, triangles);
}

void MeshData::add_sub_mesh(const MeshVertexStreams& streams, core::Span<IndexedTriangle> triangles) {
    add_sub_mesh(triangles, add_vertices_from_streams(streams));
}

u32 MeshData::add_vertices_from_streams(const MeshVertexStreams& streams) {
    y_debug_assert(!streams.is_empty());

    // AABB
    {
        math::Vec3 max(-std::numeric_limits<float>::max());
        math::Vec3 min(std::numeric_limits<float>::max());

        const core::Span<math::Vec3> positions = streams.stream<VertexStreamType::Position>();
        std::for_each(positions.begin(), positions.end(), [&](const math::Vec3& p) {
            max = max.max(p);
            min = min.min(p);
        });

        _aabb = _sub_meshes.is_empty() ? AABB(min, max) : _aabb.merged(AABB(min, max));

        y_debug_assert(math::all_finite(_aabb.min()));
        y_debug_assert(math::all_finite(_aabb.max()));
    }


    // Streams
    const u32 vertex_offset = u32(_vertex_streams.vertex_count());
    _vertex_streams = _vertex_streams.merged(streams);

    return vertex_offset;
}

void MeshData::add_sub_mesh(core::Span<IndexedTriangle> triangles, u32 vertex_offset) {
    y_debug_assert(!triangles.is_empty());

    const u32 first_triangle = u32(_triangles.size());
    _triangles.set_min_capacity(_triangles.size() + triangles.size());
    std::transform(triangles.begin(), triangles.end(), std::back_inserter(_triangles), [=](IndexedTriangle tri) {
        return IndexedTriangle {
            tri[0] + vertex_offset,
            tri[1] + vertex_offset,
            tri[2] + vertex_offset,
        };
    });

    // Sub-meshes
    _sub_meshes << SubMesh{u32(triangles.size()), first_triangle};
}

void MeshData::add_sub_mesh(core::Span<FullVertex> vertices, core::Span<IndexedTriangle> triangles) {
    add_sub_mesh(pack_vertices(vertices), triangles);
}

void MeshData::add_sub_mesh(core::Span<PackedVertex> vertices, core::Span<IndexedTriangle> triangles) {
    add_sub_mesh(MeshVertexStreams(vertices), triangles);
}

float MeshData::radius() const {
    return _aabb.origin_radius();
}

const AABB& MeshData::aabb() const {
    return _aabb;
}

const MeshVertexStreams& MeshData::vertex_streams() const {
    return _vertex_streams;
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

bool MeshData::has_skeleton() const {
    return bool(_skeleton);
}

bool MeshData::is_empty() const {
    return _vertex_streams.is_empty() || _triangles.is_empty() || _sub_meshes.is_empty();
}

}

