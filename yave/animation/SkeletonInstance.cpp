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

#include "SkeletonInstance.h"

namespace yave {

SkeletonInstance::SkeletonInstance(DevicePtr dptr, const Skeleton& skeleton) :
		_skeleton(&skeleton),
		_bone_transforms(dptr, Skeleton::max_bones),
		_descriptor_set(dptr, {Binding(_bone_transforms)}) {

	auto map = _bone_transforms.map();
	std::fill(map.begin(), map.end(), math::Transform<>());
}

SkeletonInstance::SkeletonInstance(SkeletonInstance&& other) {
	swap(other);
}

SkeletonInstance& SkeletonInstance::operator=(SkeletonInstance&& other) {
	swap(other);
	return *this;
}

void SkeletonInstance::swap(SkeletonInstance& other) {
	std::swap(_skeleton, other._skeleton);
	std::swap(_bone_transforms, other._bone_transforms);
}

void SkeletonInstance::update() {
	auto map = _bone_transforms.map();
	map[rand() % map.size()].position() += {0.0f, 0.0f, 1.0f};
}

}
