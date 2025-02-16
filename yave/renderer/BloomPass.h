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
#ifndef YAVE_RENDERER_BLOOMPASS_H
#define YAVE_RENDERER_BLOOMPASS_H

#include "DownsamplePass.h"
#include "BlurPass.h"

#include <yave/graphics/shader_structs.h>

namespace yave {

struct BloomSettings {
    float radius = 0.005f;
    usize pyramids = 4;
    float intensity = 0.1f;
};

struct BloomPass {
    FrameGraphImageId bloomed;

    static BloomPass create(FrameGraph& framegraph, FrameGraphImageId input, FrameGraphTypedBufferId<shader::ExposureParams> exposure, const BloomSettings& settings = BloomSettings());
};

}

#endif // YAVE_RENDERER_BLOOMPASS_H

