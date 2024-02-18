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
#ifndef YAVE_RENDERER_CAMERABUFFERPASS_H
#define YAVE_RENDERER_CAMERABUFFERPASS_H

#include <yave/scene/SceneView.h>
#include <yave/framegraph/FrameGraphResourceId.h>

namespace yave {

struct TAASettings {
    enum class JitterSeq {
        Weyl,
        R2,
    };

    enum class WeightingMode : u32 {
        None,
        Luminance,
        Log,
    };

    JitterSeq jitter = JitterSeq::R2;
    WeightingMode weighting_mode = WeightingMode::Log;

    float blending_factor = 0.9f;
    float jitter_intensity = 1.0f;

    bool use_clamping = true;
    bool use_motion_rejection = true;
    bool use_previous_matching = true;
    bool use_weighted_clamp = true;

    bool enable = true;
};

struct CameraBufferPass {
    SceneView view;
    SceneView unjittered_view;

    FrameGraphTypedBufferId<uniform::Camera> camera;

    TAASettings taa_settings;

    static CameraBufferPass create_no_jitter(FrameGraph&, const SceneView& view);
    static CameraBufferPass create(FrameGraph& framegraph, const SceneView& view, const math::Vec2ui& size, const TAASettings& settings = {});
};

}

#endif // YAVE_RENDERER_CAMERABUFFERPASS_H

