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

#include "TAAPass.h"

#include <yave/camera/Camera.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {

static u32 taa_flags(const TAASettings& settings) {
    u32 flags = 0;
    if(settings.use_clamping) {
        flags |= shader::TAAFeatureBits::ClampingBit;
    }
    if(settings.use_denoise) {
        flags |= shader::TAAFeatureBits::DenoiseBit;
    }
    return flags;
}


TAAPass TAAPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId in_color, FrameGraphPersistentResourceId persistent_color_id, FrameGraphPersistentResourceId persistent_motion_id, const TAASettings& settings) {
    if(!settings.enable) {
        TAAPass pass;
        pass.anti_aliased = in_color;
        return pass;
    }


    const ImageFormat format = framegraph.image_format(in_color);
    const math::Vec2ui size = framegraph.image_size(in_color);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("TAA pass");

    const auto aa = builder.declare_image(format, size);
    builder.add_input_usage(aa, ImageUsage::TextureBit);

    FrameGraphImageId prev_color = framegraph.make_persistent_and_get_prev(aa, persistent_color_id);
    if(!prev_color.is_valid()) {
        prev_color = builder.declare_copy(aa);
    }

    FrameGraphImageId prev_motion = framegraph.make_persistent_and_get_prev(gbuffer.motion, persistent_motion_id);
    if(!prev_motion.is_valid()) {
        prev_motion = builder.declare_copy(gbuffer.motion);
    }

    const u32 flags = taa_flags(settings);

    builder.add_uniform_input(in_color, SamplerType::PointClamp);
    builder.add_uniform_input(prev_color, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.motion, SamplerType::PointClamp);
    builder.add_uniform_input(prev_motion, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_inline_input(InlineDescriptor(flags));
    builder.add_storage_output(aa);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const auto& program = device_resources()[DeviceResources::TAAResolveProgram];
        recorder.dispatch_threads(program, size, self->descriptor_set());
    });

    TAAPass pass;
    pass.anti_aliased = aa;
    return pass;
}

TAAPass TAAPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId in_color, const TAASettings& settings) {
    static const FrameGraphPersistentResourceId persistent_color_id = FrameGraphPersistentResourceId::create();
    static const FrameGraphPersistentResourceId persistent_motion_id = FrameGraphPersistentResourceId::create();

    return create(framegraph, gbuffer, in_color, persistent_color_id, persistent_motion_id, settings);
}

}

