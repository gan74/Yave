/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#ifndef YAVE_MESHES_BONE_H
#define YAVE_MESHES_BONE_H

#include <yave/yave.h>
#include <yave/utils/serde.h>

namespace yave {

struct BoneTransform {
	math::Vec3 position;
	math::Vec3 scale = math::Vec3(1.0f);
	math::Quaternion<> rotation;

	math::Transform<> to_transform() const {
		return math::Transform<>(position, rotation, scale);
	}

	math::Transform<> lerp(const BoneTransform& end, float factor) const {
		float q = 1.0f - factor;
		return math::Transform<>(position * q + end.position * factor,
								 rotation.slerp(end.rotation, factor),
								 scale * q + end.scale * factor);
	}
};

static_assert(std::is_trivially_copyable_v<BoneTransform>, "BoneTransform should be trivially copyable");

struct Bone {

	y_serde2(name, parent, local_transform)

	core::String name;
	u32 parent;

	BoneTransform local_transform;

	bool has_parent() const {
		return parent != u32(-1);
	}

	math::Transform<> transform() const {
		return local_transform.to_transform();
	}
};


}


#endif // YAVE_MESHES_BONE_H
