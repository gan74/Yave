/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include "Skeleton.h"

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

[[maybe_unused]]
static void debug_bone(usize index, core::Span<Bone> bones, const core::String& indent = "") {
    log_msg(indent + bones[index].name + " (" + index + ")", Log::Debug);
    /*log_msg(indent + "{" + bones[index].local_transform.rotation.x() + ", " +
                           bones[index].local_transform.rotation.y() + ", " +
                           bones[index].local_transform.rotation.z() + ", " +
                           bones[index].local_transform.rotation.w() + "}", Log::Debug);*/

    for(usize i = 0; i != bones.size(); ++i) {
        if(bones[i].parent == index) {
            debug_bone(i, bones, indent + "  ");
        }
    }
}

Skeleton::Skeleton(core::Span<Bone> bones) : _bones(bones) {
    if(_bones.size() > max_bones) {
        y_fatal("Bone count exceeds max_bones.");
    }

    for(usize i = 0; i != _bones.size(); ++i) {
        const auto& bone = _bones[i];
        auto transform = bone.transform();
        _transforms << transform;
        _inverses << (bone.has_parent() ? _inverses[bone.parent] * transform : transform);
    }

    for(auto& transform : _inverses) {
        transform = transform.inverse();
    }

    /*for(usize i = 0; i != _bones.size(); ++i) {
        if(!_bones[i].has_parent()) {
            debug_bone(i, _bones);
        }
    }*/
}

core::Span<Bone> Skeleton::bones() const {
    return _bones;
}

core::Span<math::Transform<>> Skeleton::bone_transforms() const {
    return _transforms;
}

core::Span<math::Transform<>> Skeleton::inverse_absolute_transforms() const {
    return _inverses;
}

}

