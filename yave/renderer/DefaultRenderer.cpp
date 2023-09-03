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

#include "DefaultRenderer.h"

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

DefaultRenderer DefaultRenderer::create(FrameGraph& framegraph, const SceneView& scene_view, const math::Vec2ui& size, const RendererSettings& settings) {
    y_profile();

    static usize jitter = 1;
    const usize jitter_index = jitter++;
    const SceneView jiterred(&scene_view.world(), scene_view.camera().jittered(jitter_index, size, settings.taa.jitter_intensity));

    DefaultRenderer renderer;

    renderer.gbuffer        = GBufferPass::create(framegraph, settings.taa.enable ? jiterred : scene_view, size);
    renderer.ssao           = SSAOPass::create(framegraph, renderer.gbuffer, settings.ssao);
    renderer.lighting       = LightingPass::create(framegraph, renderer.gbuffer, renderer.ssao.ao, settings.lighting);
    renderer.atmosphere     = AtmospherePass::create(framegraph, renderer.gbuffer, renderer.lighting.lit);

    renderer.taa            = TAAPass::create(framegraph, renderer.atmosphere.lit, renderer.gbuffer.depth, renderer.gbuffer.scene_pass.camera_buffer, settings.taa);

    renderer.exposure       = ExposurePass::create(framegraph, renderer.taa.anti_aliased);
    renderer.bloom          = BloomPass::create(framegraph, renderer.taa.anti_aliased, renderer.exposure.params, settings.bloom);
    renderer.tone_mapping   = ToneMappingPass::create(framegraph, renderer.bloom.bloomed, renderer.exposure, settings.tone_mapping);

    renderer.final = renderer.tone_mapping.tone_mapped;
    renderer.depth = renderer.gbuffer.depth;

    return renderer;
}

}

