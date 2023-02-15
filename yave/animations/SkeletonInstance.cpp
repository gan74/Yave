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

#include "SkeletonInstance.h"

namespace yave {

SkeletonInstance::SkeletonInstance(const Skeleton* skeleton) :
        _skeleton(skeleton),
        _bone_transforms(new std::array<math::Transform<>, Skeleton::max_bones>()),
        _bone_transform_buffer(Skeleton::max_bones),
        _descriptor_set(Descriptor(_bone_transform_buffer)) {

    flush_data();
}

void SkeletonInstance::animate(const AssetPtr<Animation>& anim) {
    _animation = anim;
    _anim_timer.reset();
}

void SkeletonInstance::update() {
    if(!_animation) {
        return;
    }

    const float time = std::fmod(float(_anim_timer.elapsed().to_secs()), _animation->duration());

    const auto& anim = *_animation;
    const auto& bones = _skeleton->bones();
    const auto& bone_transforms = _skeleton->bone_transforms();
    const auto& invs = _skeleton->inverse_absolute_transforms();

    auto& out_transforms = *_bone_transforms;


    for(usize i = 0; i != bones.size(); ++i) {
        const auto& bone = bones[i];
        auto bone_tr = anim.bone_transform(bone.name, time).value_or(bone_transforms[i]);

        out_transforms[i] = (bone.has_parent() ? out_transforms[bone.parent] * bone_tr : bone_tr);
    }
    for(usize i = 0; i != bones.size(); ++i) {
        out_transforms[i] *= invs[i];
    }

    flush_data();
}

void SkeletonInstance::flush_data() {
    auto map = _bone_transform_buffer.map(MappingAccess::WriteOnly);
    std::copy(_bone_transforms->begin(), _bone_transforms->end(), map.begin());
}

}

