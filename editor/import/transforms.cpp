/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include "transforms.h"

namespace editor {
namespace import {

template<typename T>
std::remove_const_t<std::remove_reference_t<T>> copy(T&& t) {
	return t;
}

static math::Vec3 h_transform(const math::Vec3& v, const math::Transform<>& tr) {
	auto h = tr * math::Vec4(v, 1.0f);
	return h.to<3>() / h.w();
}

static math::Vec3 transform(const math::Vec3& v, const math::Transform<>& tr) {
	return tr.to<3, 3>() * v;
}

static Vertex transform(const Vertex& v, const math::Transform<>& tr) {
	return Vertex {
			h_transform(v.position, tr),
			transform(v.normal, tr),
			transform(v.tangent, tr),
			v.uv
		};
}

static BoneTransform transform(const BoneTransform& bone, const math::Transform<>& tr) {
	auto [pos, rot, scale] = tr.decompose();
	return BoneTransform {
			bone.position + pos,
			bone.scale * scale,
			bone.rotation * rot
		};
}


MeshData transform(const MeshData& mesh, const math::Transform<>& tr) {
	auto vertices = core::vector_with_capacity<Vertex>(mesh.vertices().size());
	std::transform(mesh.vertices().begin(), mesh.vertices().end(), std::back_inserter(vertices), [=](const auto& vert) { return transform(vert, tr); });

	if(mesh.has_skeleton()) {
		auto bones = core::vector_with_capacity<Bone>(mesh.bones().size());
		std::transform(mesh.bones().begin(), mesh.bones().end(), std::back_inserter(bones), [=](const auto& bone) {
				return bone.has_parent() ? bone : Bone{bone.name, bone.parent, transform(bone.local_transform, tr)};
			});

		return MeshData::from_parts(std::move(vertices), copy(mesh.triangles()), copy(mesh.skin()), std::move(bones));
	}

	return MeshData::from_parts(std::move(vertices), copy(mesh.triangles()));
}




static AnimationChannel set_speed(const AnimationChannel& anim, float speed) {
	auto keys = core::vector_with_capacity<AnimationChannel::BoneKey>(anim.keys().size());
	std::transform(anim.keys().begin(), anim.keys().end(), std::back_inserter(keys), [=](const auto& key){
			return AnimationChannel::BoneKey{key.time / speed, key.local_transform};
		});

	return AnimationChannel(anim.name(), std::move(keys));
}

Animation set_speed(const Animation& anim, float speed) {
	auto channels = core::vector_with_capacity<AnimationChannel>(anim.channels().size());
	std::transform(anim.channels().begin(), anim.channels().end(), std::back_inserter(channels), [=](const auto& channel){ return set_speed(channel, speed); });

	return Animation(anim.duration() / speed, std::move(channels));
}

}
}
