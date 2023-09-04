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
#ifndef YAVE_RENDERER_TAAPASS_H
#define YAVE_RENDERER_TAAPASS_H

#include "ExposurePass.h"

namespace yave {

struct TAASettings {
    bool enable = true;
    bool use_reprojection = true;
    bool use_clamping = true;
    bool use_mask = true;

    float blending_factor = 0.9f;
    float jitter_intensity = 1.0f;
};

struct TAAJitterPass {
    SceneView jittered_view;
    SceneView unjittered_view;
    TAASettings settings;

    Y_TODO(why not move the camera buffer generation here)

    static TAAJitterPass create(FrameGraph&, const SceneView& view, const math::Vec2ui& size, const TAASettings& settings = {});
};

struct TAAPass {
    FrameGraphImageId anti_aliased;
    FrameGraphImageId deocclusion_mask;

    static TAAPass create(FrameGraph& framegraph,
                          const TAAJitterPass& jitter,
                          FrameGraphImageId in_color,
                          FrameGraphImageId in_depth,
                          FrameGraphImageId& in_motion,
                          FrameGraphTypedBufferId<uniform::Camera> camera_buffer);
};


}

#endif // YAVE_RENDERER_TAAPASS_H

