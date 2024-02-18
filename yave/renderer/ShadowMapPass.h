/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef YAVE_RENDERER_SHADOWMAPPASS_H
#define YAVE_RENDERER_SHADOWMAPPASS_H

#include "GBufferPass.h"

#include <y/core/HashMap.h>

namespace yave {

enum class ShadowMapSpillPolicy {
    DownSample,
    Discard,
};

struct ShadowMapSettings {
    u32 shadow_map_size = 2048;
    usize shadow_atlas_size = 4;
    ShadowMapSpillPolicy spill_policy = ShadowMapSpillPolicy::DownSample;
};

struct ShadowMapPass {
    FrameGraphImageId shadow_map;
    FrameGraphTypedBufferId<uniform::ShadowMapParams> shadow_params;

    std::shared_ptr<core::FlatHashMap<u64, math::Vec4ui>> shadow_indices;

    static ShadowMapPass create(FrameGraph& framegraph, const SceneView& scene, const ShadowMapSettings& settings = ShadowMapSettings());
};


}


#endif // YAVE_RENDERER_SHADOWMAPPASS_H

