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

#include "TAAPass.h"

#include <yave/camera/Camera.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {

TAAPass TAAPass::create(FrameGraph& framegraph,
                        const CameraBufferPass& camera,
                        FrameGraphImageId in_color,
                        FrameGraphImageId in_depth,
                        FrameGraphImageId in_motion) {

    const TAASettings& settings = camera.taa_settings;

    if(!settings.enable) {
        TAAPass pass;
        pass.anti_aliased = in_color;
        return pass;
    }

    const ImageFormat format = framegraph.image_format(in_color);
    const math::Vec2ui size = framegraph.image_size(in_color);

    FrameGraphPassBuilder builder = framegraph.add_pass("TAA resolve pass");

    const auto aa = builder.declare_image(format, size);
    builder.add_input_usage(aa, ImageUsage::TextureBit);

    static const FrameGraphPersistentResourceId color_id = FrameGraphPersistentResourceId::create();
    FrameGraphImageId prev_color = framegraph.make_persistent_and_get_prev(aa, color_id);
    if(!prev_color.is_valid()) {
        prev_color = builder.declare_copy(aa);
    }

    static const FrameGraphPersistentResourceId motion_id = FrameGraphPersistentResourceId::create();
    FrameGraphImageId prev_motion = framegraph.make_persistent_and_get_prev(in_motion, motion_id);
    if(!prev_motion.is_valid()) {
        prev_motion = builder.declare_copy(in_motion);
    }

    const u32 clamping_bit              = 0x01;
    const u32 motion_rejection_bit      = 0x02;

    u32 flag_bits = 0;
    flag_bits |= (settings.use_clamping ? clamping_bit : 0);
    flag_bits |= (settings.use_motion_rejection ? motion_rejection_bit : 0);

    struct SettingsData {
        u32 flags;
        u32 weighting_mode;
        float blending_factor;
    } settings_data {
        flag_bits,
        u32(settings.weighting_mode),
        settings.blending_factor,
    };

    builder.add_color_output(aa);
    builder.add_uniform_input(in_depth, SamplerType::PointClamp);
    builder.add_uniform_input(in_color, SamplerType::PointClamp);
    builder.add_uniform_input(prev_color);
    builder.add_uniform_input(in_motion);
    builder.add_uniform_input(prev_motion);
    builder.add_uniform_input(camera.camera);
    builder.add_inline_input(InlineDescriptor(settings_data));
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        const auto* material = device_resources()[DeviceResources::TAAResolveMaterialTemplate];
        render_pass.bind_material_template(material, self->descriptor_sets());
        render_pass.draw_array(3);
    });


    TAAPass pass;
    pass.anti_aliased = aa;
    return pass;
}

}

