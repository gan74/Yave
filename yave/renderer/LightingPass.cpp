/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

#include "LightingPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/scene/Scene.h>

#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/ecs/EntityWorld.h>

namespace yave {

LightingPass LightingPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, const LightClusterPass& cluster, const LightingSettings& settings) {
    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Lighting pass");

    const auto lit = builder.declare_copy(gbuffer.emissive);
    const math::Vec2ui size = framegraph.image_size(lit);

    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(gbuffer.color);
    builder.add_uniform_input(gbuffer.normal);
    builder.add_uniform_input(cluster.shadow_pass.shadow_map, SamplerType::Shadow);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_storage_input(cluster.directionals);
    builder.add_storage_input(cluster.points);
    builder.add_storage_input(cluster.spots);
    builder.add_storage_input(cluster.shadow_pass.shadow_infos);
    builder.add_uniform_input(cluster.cluster_info);
    builder.add_storage_input(cluster.tiles);

    builder.add_storage_output(lit);

    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const auto& program = device_resources()[settings.debug_tiles ? DeviceResources::DeferredSingleDebugPassProgram : DeviceResources::DeferredSinglePassProgram];
        recorder.dispatch_threads(program, size, self->descriptor_set());
    });

    LightingPass pass;
    pass.lit = lit;
    return pass;
}

}

