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
#ifndef YAVE_ANIMATIONS_SKELETONINSTANCE_H
#define YAVE_ANIMATIONS_SKELETONINSTANCE_H

#include <y/core/Chrono.h>

#include <yave/assets/AssetPtr.h>
#include <yave/meshes/Skeleton.h>
#include <yave/graphics/buffers/buffers.h>
#include <yave/graphics/descriptors/DescriptorSet.h>

#include "Animation.h"

namespace yave {

class SkeletonInstance {

    public:
        SkeletonInstance() = default;

        // this seems unsafe...
        SkeletonInstance(const Skeleton* skeleton);

        void animate(const AssetPtr<Animation>& anim);

        void update();

        const auto& descriptor_set() const {
            return _descriptor_set;
        }

    private:
        void flush_data();

        const Skeleton* _skeleton = nullptr;
        std::unique_ptr<std::array<math::Transform<>, Skeleton::max_bones>> _bone_transforms;

        TypedUniformBuffer<math::Transform<>, MemoryType::CpuVisible> _bone_transform_buffer;
        DescriptorSet _descriptor_set;

        AssetPtr<Animation> _animation;
        core::Chrono _anim_timer;

};

}

#endif // YAVE_ANIMATIONS_SKELETONINSTANCE_H

