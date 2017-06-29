/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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

#include "SkeletonDebug.h"

#include <yave/material/Material.h>
#include <y/io/File.h>

namespace yave {

static Material create_material(const DescriptorSet& descriptor_set) {
	return Material(descriptor_set.device(), MaterialData()
			.set_primitive_type(PrimitiveType::Lines)
			.set_frag_data(SpirVData::from_file(io::File::open("skeleton.frag.spv").expected("Unable to load spirv file")))
			.set_vert_data(SpirVData::from_file(io::File::open("skeleton.vert.spv").expected("Unable to load spirv file")))
		);
}

static math::Transform<> bone_transform(u32 bone_index, const Skeleton& skeleton) {
	const auto& bone = skeleton.bones()[bone_index];
	return bone.has_parent() ? math::Transform<>(bone_transform(bone.parent, skeleton) * bone.transform) : bone.transform;
}


SkeletonDebug::SkeletonDebug(DevicePtr dptr, const AssetPtr<SkinnedMesh>& mesh) :
		_mesh(mesh),
		_bone_transforms(dptr, Skeleton::max_bones * 2),
		_descriptor_set(dptr, {Binding(_bone_transforms)}),
		_material(create_material(_descriptor_set)) {

	set_radius(_mesh->radius());


	auto map = _bone_transforms.map();

	auto skeleton = mesh->skeleton();
	for(usize i = 0; i != skeleton.bones().size(); ++i) {
		const auto& bone = mesh->skeleton().bones()[i];
		map[i * 2] = bone.has_parent() ? bone_transform(bone.parent, skeleton) : math::Transform<>();
		map[i * 2 + 1] = bone_transform(u32(i), skeleton);
	}
}

SkeletonDebug::SkeletonDebug(SkeletonDebug&& other) :
		Renderable(other),
		_mesh(other._mesh),
		_bone_transforms(std::move(other._bone_transforms)),
		_descriptor_set(std::move(other._descriptor_set)),
		_material(std::move(other._material)) {
}

void SkeletonDebug::render(const FrameToken&, CmdBufferRecorderBase& recorder, const SceneData& scene_data) const {
	recorder.bind_material(*_material, {scene_data.descriptor_set, _descriptor_set});
	recorder.bind_attribs(scene_data.instance_attribs);
	recorder.draw(_mesh->skeleton().bones().size() * 2);

	/*for(usize i = 0; i != 2; ++i) {
		const auto& bone = _mesh->skeleton().bones()[i];
		log_msg(core::str(i) + (": " + bone.name));
		log_msg("\t"_s + bone.transform.position().x() + ", " + bone.transform.position().y() + ", " + bone.transform.position().z());
		log_msg("");
	}*/
}

}
