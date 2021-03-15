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

#include "DefaultRenderer.h"

namespace yave {

DefaultRenderer DefaultRenderer::create(FrameGraph& framegraph, const SceneView& view, const math::Vec2ui& size, const RendererSettings& settings) {
    y_profile();

    DefaultRenderer renderer;

    renderer.gbuffer        = GBufferPass::create(framegraph, view, size);
    renderer.ssao           = SSAOPass::create(framegraph, renderer.gbuffer, settings.ssao);
    renderer.lighting       = LightingPass::create(framegraph, renderer.gbuffer, renderer.ssao.ao, settings.lighting);
    renderer.atmosphere     = AtmospherePass::create(framegraph, renderer.gbuffer, renderer.lighting.lit);
    renderer.bloom          = BloomPass::create(framegraph, renderer.atmosphere.lit, settings.bloom);
    renderer.tone_mapping   = ToneMappingPass::create(framegraph, renderer.bloom.bloomed, settings.tone_mapping);


    renderer.final = renderer.tone_mapping.tone_mapped;
    renderer.depth = renderer.gbuffer.depth;

    return renderer;
}

}

