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

#include "mesh_utils.h"

#include <yave/meshes/MeshData.h>
#include <yave/animations/Animation.h>

#include <y/core/ScratchPad.h>
#include <y/utils/log.h>


namespace editor {
namespace import {

static MeshVertexStreams copy(const MeshVertexStreams& streams) {
    return streams.merged(MeshVertexStreams());
}

static math::Vec3 transform_position(const math::Vec3& v, const math::Transform<>& tr) {
    const auto h = tr * math::Vec4(v, 1.0f);
    return h.to<3>() / h.w();
}

static math::Vec3 transform_direction(const math::Vec3& v, const math::Transform<>& tr) {
    return tr.to<3, 3>() * v;
}

[[maybe_unused]] static FullVertex transform(const FullVertex& v, const math::Transform<>& tr) {
    return FullVertex {
        transform_position(v.position, tr),
        transform_direction(v.normal, tr).normalized(),
        math::Vec4(transform_direction(v.tangent.to<3>(), tr).normalized(), v.tangent.w()),
        v.uv
    };
}

[[maybe_unused]] static PackedVertex transform(const PackedVertex& v, const math::Transform<>& tr) {
    return pack_vertex(transform(unpack_vertex(v), tr));

}

[[maybe_unused]] static BoneTransform transform(const BoneTransform& bone, const math::Transform<>& tr) {
    auto [pos, rot, scale] = tr.decompose();
    return BoneTransform {
            bone.position + pos,
            bone.scale * scale,
            bone.rotation * rot
        };
}


MeshData transform(const MeshData& mesh, const math::Transform<>& tr) {
    y_profile();

    MeshVertexStreams vertex_streams = copy(mesh.vertex_streams());
    for(auto& pos : vertex_streams.stream<VertexStreamType::Position>()) {
        pos = transform_position(pos, tr);
    }
    for(auto& packed : vertex_streams.stream<VertexStreamType::NormalTangent>()) {
        PackedVertex vert = {};
        vert.packed_normal = packed.x();
        vert.packed_tangent_sign = packed.y();
        const PackedVertex transformed = transform(vert, tr);
        packed.x() = transformed.packed_normal;
        packed.y() = transformed.packed_tangent_sign;
    }

    if(mesh.has_skeleton()) {
        auto bones = core::Vector<Bone>::with_capacity(mesh.bones().size());
        std::transform(mesh.bones().begin(), mesh.bones().end(), std::back_inserter(bones), [=](const auto& bone) {
                return bone.has_parent() ? bone : Bone{bone.name, bone.parent, transform(bone.local_transform, tr)};
            });

        return MeshData(std::move(vertex_streams), mesh.triangles());
    }

    return MeshData(std::move(vertex_streams), mesh.triangles());
}

MeshData compute_tangents(const MeshData& mesh) {
    y_profile();

    auto vertex_streams = copy(mesh.vertex_streams());
    const auto positions = vertex_streams.stream<VertexStreamType::Position>();
    const auto tex_coords = vertex_streams.stream<VertexStreamType::Uv>();

    core::FixedArray<math::Vec3> tangents(vertex_streams.vertex_count());
    for(IndexedTriangle tri : mesh.triangles()) {
        const math::Vec3 edges[] = {positions[tri[1]] - positions[tri[0]], positions[tri[2]] - positions[tri[0]]};
        const math::Vec2 uvs[] = {tex_coords[tri[0]], tex_coords[tri[1]], tex_coords[tri[2]]};
        const float dt[] = {uvs[1].y() - uvs[0].y(), uvs[2].y() - uvs[0].y()};
        math::Vec3 ta = -((edges[0] * dt[1]) - (edges[1] * dt[0])).normalized();
        tangents[tri[0]] += ta;
        tangents[tri[1]] += ta;
        tangents[tri[2]] += ta;
    }

    auto packed = vertex_streams.stream<VertexStreamType::NormalTangent>();
    for(usize i = 0; i != tangents.size(); ++i) {
        Y_TODO(Compute bitangent too)
        packed[i].y() = pack_2_10_10_10(tangents[i].normalized());
    }

    return MeshData(std::move(vertex_streams), mesh.triangles());
}

static AnimationChannel set_speed(const AnimationChannel& anim, float speed) {
    y_profile();
    auto keys = core::Vector<AnimationChannel::BoneKey>::with_capacity(anim.keys().size());
    std::transform(anim.keys().begin(), anim.keys().end(), std::back_inserter(keys), [=](const auto& key){
        return AnimationChannel::BoneKey{key.time / speed, key.local_transform};
    });

    return AnimationChannel(anim.name(), std::move(keys));
}

Animation set_speed(const Animation& anim, float speed) {
    y_profile();
    auto channels = core::Vector<AnimationChannel>::with_capacity(anim.channels().size());
    std::transform(anim.channels().begin(), anim.channels().end(), std::back_inserter(channels), [=](const auto& channel){ return set_speed(channel, speed); });

    return Animation(anim.duration() / speed, std::move(channels));
}

core::Result<void> export_to_obj(const MeshData& mesh, io2::Writer& writer) {
    auto write = [&](std::string_view l) {
        return writer.write(l.data(), l.size());
    };

    y_try_discard(write("# Yave OBJ export\n"));
    y_try_discard(write("o Mesh\n"));

    const usize vertex_count = mesh.vertex_streams().vertex_count();
    for(usize i = 0; i != vertex_count; ++i) {
        const FullVertex v = unpack_vertex(mesh.vertex_streams()[i]);
        y_try_discard(write(fmt("v % % %\nvn % % %\nvt % %\n",
            v.position.x(),
            v.position.y(),
            v.position.z(),
            v.normal.x(),
            v.normal.y(),
            v.normal.z(),
            v.uv.x(),
            v.uv.y()
        )));
    }

    y_try_discard(write("s 0\n"));

    for(const IndexedTriangle& triangle : mesh.triangles()) {
        const IndexedTriangle tri = {triangle[0] + 1, triangle[1] + 1, triangle[2] + 1};
        y_try_discard(write(fmt("f %/%/% %/%/% %/%/%\n",
            tri[0], tri[0], tri[0],
            tri[1], tri[1], tri[1],
            tri[2], tri[2], tri[2]
        )));
    }
    return core::Ok();
}

}
}

