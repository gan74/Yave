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

#include "transforms.h"

namespace editor {
namespace import {

template<typename T>
core::Vector<T> copy(core::Span<T> t) {
	return core::Vector<T>(t);
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

		return MeshData(std::move(vertices), copy(mesh.triangles()), copy(mesh.skin()), std::move(bones));
	}

	return MeshData(std::move(vertices), copy(mesh.triangles()));
}

MeshData compute_tangents(const MeshData& mesh) {
	core::Vector<Vertex> vertices = copy(mesh.vertices());
	core::Vector<IndexedTriangle> triangles = copy(mesh.triangles());

	for(IndexedTriangle tri : triangles) {
		math::Vec3 edges[] = {vertices[tri[1]].position - vertices[tri[0]].position, vertices[tri[2]].position - vertices[tri[0]].position};
		math::Vec2 uvs[] = {vertices[tri[0]].uv, vertices[tri[1]].uv, vertices[tri[2]].uv};
		float dt[] = {uvs[1].y() - uvs[0].y(), uvs[2].y() - uvs[0].y()};
		math::Vec3 ta = -((edges[0] * dt[1]) - (edges[1] * dt[0])).normalized();
		vertices[tri[0]].tangent += ta;
		vertices[tri[1]].tangent += ta;
		vertices[tri[2]].tangent += ta;
	}

	for(Vertex& v : vertices) {
		v.tangent.normalize();
	}

	return MeshData(std::move(vertices), std::move(triangles), copy(mesh.skin()), copy(mesh.bones()));
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

ImageData compute_mipmaps(const ImageData& image) {
	if(image.layers() != 1 || image.size().z() != 1) {
		y_fatal("Only one layer is supported.");
	}
	if(image.format() != ImageFormat(vk::Format::eR8G8B8A8Unorm)) {
		y_fatal("Only RGBA is supported.");
	}

	const ImageFormat format(vk::Format::eR8G8B8A8Unorm);
	const usize components = 4;

	usize mip_count = ImageData::mip_count(image.size());
	y_debug_assert(mip_count >= 1);
	usize data_size = ImageData::layer_byte_size(image.size(), format, mip_count);
	std::unique_ptr<u8[]> data = std::make_unique<u8[]>(data_size);

	auto compute_mip = [components](const u8* image_data, u8* output, const math::Vec2ui& orig_size) -> usize {
			usize cursor = 0;
			math::Vec2ui mip_size = {std::max(1u, orig_size.x() / 2),
									 std::max(1u, orig_size.y() / 2)};
			usize row_size = orig_size.x();

			for(usize y = 0; y != mip_size.y(); ++y) {
				for(usize x = 0; x != mip_size.x(); ++x) {
					usize orig = (x * 2 + y * 2 * row_size);
					for(usize c = 0; c != components; ++c) {
						u32 acc  = image_data[components * (orig) + c];
						acc		+= image_data[components * (orig + 1) + c];
						acc		+= image_data[components * (orig + row_size) + c];
						acc		+= image_data[components * (orig + row_size + 1) + c];
						output[cursor++] = std::min(acc / 4, 0xFFu);
					}
				}
			}
			return cursor;
		};



	u8* image_data = data.get();
	usize mip_byte_size = image.byte_size(0);
	std::memcpy(image_data, image.data(), mip_byte_size);
	for(usize mip = 0; mip < mip_count; ++mip) {
		usize s = compute_mip(image_data, image_data + mip_byte_size, image.size(mip).to<2>());
		image_data += mip_byte_size;
		mip_byte_size = s;
	}
	y_debug_assert(image_data <= data.get() + data_size);
	y_debug_assert(image_data == data.get() + data_size);

	return ImageData(image.size().to<2>(), data.get(), format, mip_count);
}

}
}
