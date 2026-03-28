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
#include "ForwardPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/scene/Scene.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/graphics/images/IBLProbe.h>

#include <yave/components/SkyLightComponent.h>

namespace yave {

ForwardPass ForwardPass::create(FrameGraph& framegraph, FrameGraphImageId in_depth, FrameGraphImageId in_lit, const CameraBufferPass& camera, const LightClusterPass& cluster, const SceneVisibilitySubPass& visibility) {
    FrameGraphPassBuilder builder = framegraph.add_pass("Forward pass");

    const auto depth = builder.declare_copy(in_depth);
    const auto lit = builder.declare_copy(in_lit);

    auto [ibl_probe, intensity, sky] = visibility.visible->ibl_probe();
    struct Params {
            u32 display_sky;
            float ibl_intensity;
    } params {
        sky ? 1u : 0u,
        intensity
    };

    ForwardPass pass;
    pass.lit = lit;
    pass.scene_pass = SceneRenderSubPass::create(builder, camera, visibility, PassType::Forward);

    {
        builder.add_uniform_input(cluster.shadow_pass.shadow_map, SamplerType::Shadow, PipelineStage::None, pass.scene_pass.descriptor_set_index);
        builder.add_storage_input(cluster.directionals, PipelineStage::None, pass.scene_pass.descriptor_set_index);
        builder.add_storage_input(cluster.points, PipelineStage::None, pass.scene_pass.descriptor_set_index);
        builder.add_storage_input(cluster.spots, PipelineStage::None, pass.scene_pass.descriptor_set_index);
        builder.add_storage_input(cluster.shadow_pass.shadow_infos, PipelineStage::None, pass.scene_pass.descriptor_set_index);
        builder.add_uniform_input(cluster.cluster_info, PipelineStage::None, pass.scene_pass.descriptor_set_index);
        builder.add_storage_input(cluster.tiles, PipelineStage::None, pass.scene_pass.descriptor_set_index);
    }

    {
        builder.add_external_input(*ibl_probe);
        builder.add_external_input(Descriptor(device_resources().brdf_lut(), SamplerType::LinearClamp));
        builder.add_inline_input(params);
    }

    builder.add_depth_output(depth);
    builder.add_color_output(lit);

    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        pass.scene_pass.render(render_pass, self);
    });

    return pass;
}

}

