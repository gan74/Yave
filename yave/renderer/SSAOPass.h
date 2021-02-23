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
#ifndef YAVE_RENDERER_SSAOPASS_H
#define YAVE_RENDERER_SSAOPASS_H

#include "GBufferPass.h"

#include <y/core/Vector.h>

namespace yave {

struct SSAOSettings {
    enum class SSAOMethod {
        None,
        MiniEngine,
    };

    SSAOMethod method = SSAOMethod::MiniEngine;

    usize level_count = 3;

    float blur_tolerance = 4.6f;
    float upsample_tolerance = 12.0f;
    float noise_filter_tolerance = 0.0f;
};

struct SSAOPass {
    FrameGraphImageId linear_depth;

    FrameGraphImageId ao;

    static SSAOPass create(FrameGraph& framegraph, const GBufferPass& gbuffer, const SSAOSettings& settings = SSAOSettings());
};


}

#endif // YAVE_RENDERER_SSAOPASS_H

