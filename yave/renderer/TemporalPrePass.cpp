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

#include "TemporalPrePass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/device/DeviceResources.h>

namespace yave {


TemporalPrePass TemporalPrePass::create(FrameGraph& framegraph, const GBufferPass& gbuffer) {
    TemporalPrePass pass;
    {
        pass.motion = gbuffer.motion;
        pass.camera = gbuffer.scene_pass.camera;
    }

    static const FrameGraphPersistentResourceId depth_persistent_id = FrameGraphPersistentResourceId::create();
    static const FrameGraphPersistentResourceId motion_persistent_id = FrameGraphPersistentResourceId::create();
    const FrameGraphImageId prev_depth = framegraph.make_persistent_and_get_prev(gbuffer.depth, depth_persistent_id);
    const FrameGraphImageId prev_motion = framegraph.make_persistent_and_get_prev(gbuffer.motion, motion_persistent_id);

    if(!prev_depth.is_valid() || !prev_motion.is_valid()) {
        return pass;
    }

    FrameGraphPassBuilder builder = framegraph.add_pass("Temporal mask pass");

    const math::Vec2ui size = framegraph.image_size(gbuffer.depth);
    const auto mask = builder.declare_image(VK_FORMAT_R8_UINT, size);

    builder.add_uniform_input(gbuffer.depth, SamplerType::PointClamp);
    builder.add_uniform_input(prev_depth, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.motion, SamplerType::PointClamp);
    builder.add_uniform_input(prev_motion, SamplerType::PointClamp);
    builder.add_uniform_input(pass.camera);
    builder.add_color_output(mask);
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        const auto* material = device_resources()[DeviceResources::TemporalMaskMaterialTemplate];
        render_pass.bind_material_template(material, self->descriptor_sets());
        render_pass.draw_array(3);
    });

    pass.mask = mask;
    return pass;
}

}

