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
#ifndef YAVE_RENDERER_DEFAULTRENDERER_H
#define YAVE_RENDERER_DEFAULTRENDERER_H

#include "LightingPass.h"
#include "AtmospherePass.h"
#include "ExposurePass.h"
#include "ToneMappingPass.h"
#include "SSAOPass.h"
#include "BloomPass.h"
#include "TAAPass.h"
#include "RTAOPass.h"

namespace yave {

struct RendererSettings {
    ToneMappingSettings tone_mapping;
    LightingSettings lighting;
    SSAOSettings ssao;
    BloomSettings bloom;
    TAASettings taa;
};

struct DefaultRenderer {
    SceneVisibilitySubPass visibility;
    CameraBufferPass camera;
    GBufferPass gbuffer;
    LightingPass lighting;
    AtmospherePass atmosphere;
    ExposurePass exposure;
    BloomPass bloom;
    ToneMappingPass tone_mapping;
    SSAOPass ssao;
    TAAPass taa;

    RTAOPass rtao;

    FrameGraphImageId final;
    FrameGraphImageId depth;

    static DefaultRenderer create(FrameGraph& framegraph,
                                  const SceneView& scene_view,
                                  const math::Vec2ui& size,
                                  const RendererSettings& settings = RendererSettings());
};

}

#endif // YAVE_RENDERER_DEFAULTRENDERER_H

