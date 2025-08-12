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
#ifndef YAVE_RENDERER_AOPASS_H
#define YAVE_RENDERER_AOPASS_H

#include "GBufferPass.h"
#include "TAAPass.h"

#include <y/core/Vector.h>

namespace yave {

struct AOSettings {
    enum class AOMethod {
        MiniEngine,

        RTAOFallback = MiniEngine,
        RTAO,

        None,
    };

    AOMethod method = AOMethod::RTAO;

    struct {
        u32 level_count = 2;
        float blur_tolerance = 4.6f;
        float upsample_tolerance = 12.0f;
        float noise_filter_tolerance = 0.0f;
    } mini_engine;

    struct {
        u32 ray_count = 1;
        float max_dist = 2.0f;

        float filter_sigma = 4.0f;

        bool temporal = true;
    } rtao;
};

struct AOPass {
    FrameGraphImageId ao;

    static AOPass create(FrameGraph& framegraph, const GBufferPass& gbuffer, const AOSettings& settings = AOSettings());
};


}

#endif // YAVE_RENDERER_AOPASS_H

