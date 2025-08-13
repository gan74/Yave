/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include "GBufferPass.h"

namespace yave {

struct TAASettings {
    bool use_clamping = true;
    bool use_denoise = false;

    float clamping_range = 1.0f;
    float anti_flicker_strength = 1.0f;

    bool enable = true;
};


struct TAAPass {
    FrameGraphImageId anti_aliased;

    static TAAPass create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId in_color, FrameGraphPersistentResourceId persistent_color_id, FrameGraphPersistentResourceId persistent_motion_id, const TAASettings& settings = {});
    static TAAPass create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId in_color, const TAASettings& settings = {});
};


}

#endif // YAVE_RENDERER_TAAPASS_H

