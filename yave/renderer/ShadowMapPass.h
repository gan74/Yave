/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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

#include <yave/graphics/images/IBLProbe.h>

#include "GBufferPass.h"

#include <y/core/HashMap.h>

namespace yave {

struct ShadowMapPassSettings {
    usize shadow_map_size = 1024;
    usize shadow_atlas_size = 8;
};

struct ShadowMapPass {
    using ShadowData = uniform::ShadowMapParams;

    struct SubPass {
        SceneRenderSubPass scene_pass;
        math::Vec2ui viewport_offset;
        u32 viewport_size;
    };

    struct SubPassData {
        core::Vector<SubPass> passes;
        core::ExternalHashMap<u64, ShadowData> lights;
    };

    FrameGraphImageId shadow_map;

    std::shared_ptr<SubPassData> sub_passes;

    static ShadowMapPass create(FrameGraph& framegraph, const SceneView& scene, const ShadowMapPassSettings& settings = ShadowMapPassSettings());
};


}


#endif // YAVE_RENDERER_SHADOWMAPPASS_H

