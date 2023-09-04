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
#include <y/utils/log.h>

namespace yave {

TAAJitterPass TAAJitterPass::create(FrameGraph&, const SceneView& view, const math::Vec2ui& size, const TAASettings& settings) {
    TAAJitterPass pass;
    pass.settings = settings;
    pass.unjittered_view = view;

    if(pass.settings.enable) {
        static usize jitter = 1;
        const usize jitter_index = jitter++;
        pass.jittered_view = SceneView(&view.world(), view.camera().jittered(jitter_index, size, settings.jitter_intensity));
    } else {
        pass.jittered_view = view;
    }

    return pass;
}

TAAPass TAAPass::create(FrameGraph& framegraph,
                        const TAAJitterPass& jitter,
                        FrameGraphImageId in_color,
                        FrameGraphImageId in_depth,
                        FrameGraphImageId& in_motion,
                        FrameGraphTypedBufferId<uniform::Camera> camera_buffer) {

    const TAASettings& settings = jitter.settings;

    if(!settings.enable) {
        TAAPass pass;
        pass.anti_aliased = in_color;
        return pass;
    }

    const auto region = framegraph.region("TAA");

    const ImageFormat format = framegraph.image_format(in_color);
    const math::Vec2ui size = framegraph.image_size(in_color);


    TAAPass pass;

    if(settings.use_mask) {
        FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Deocclusion mask pass");

        const auto mask = builder.declare_image(VK_FORMAT_R8_UNORM, size);

        builder.add_image_output_usage(mask, ImageUsage::TransferDstBit);

        builder.add_storage_output(mask);
        builder.add_uniform_input(in_motion);
        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            recorder.clear_image(self->resources().image<ImageUsage::TransferDstBit>(mask));
            const auto& program = device_resources()[DeviceResources::TAADeocclusionMaskProgram];
            recorder.dispatch_size(program, size, self->descriptor_sets());
        });

        pass.deocclusion_mask = mask;
    }

    {
        static const FrameGraphPersistentResourceId color_id = FrameGraphPersistentResourceId::create();
        static const FrameGraphPersistentResourceId camera_id = FrameGraphPersistentResourceId::create();

        FrameGraphPassBuilder builder = framegraph.add_pass("TAA resolve pass");

        const auto aa = builder.declare_image(format, size);

        FrameGraphImageId prev_color = framegraph.make_persistent_and_get_prev(aa, color_id);
        if(!prev_color.is_valid()) {
            prev_color = builder.declare_copy(aa);
        }
        builder.add_image_input_usage(aa, ImageUsage::TextureBit);

        FrameGraphBufferId prev_camera = framegraph.make_persistent_and_get_prev(camera_buffer, camera_id);
        if(!prev_camera.is_valid()) {
            prev_camera = builder.declare_copy(camera_buffer);
        }

        const u32 reprojection_bit = 0x1;
        const u32 clamping_bit = 0x2;
        const u32 mask_bit = 0x4;

        u32 flag_bits = 0;
        flag_bits |= (settings.use_reprojection ? reprojection_bit : 0);
        flag_bits |= (settings.use_clamping ? clamping_bit : 0);
        flag_bits |= (settings.use_mask ? mask_bit : 0);

        struct SettingsData {
            u32 flags;
            float blending_factor;
        } settings_data {
            flag_bits,
            settings.blending_factor,
        };

        builder.add_color_output(aa);
        builder.add_uniform_input(in_depth, SamplerType::PointClamp);
        builder.add_uniform_input(in_color, SamplerType::PointClamp);
        builder.add_uniform_input(prev_color);
        builder.add_uniform_input(in_motion);
        builder.add_uniform_input_with_default(pass.deocclusion_mask, *device_resources()[DeviceResources::BlackTexture]);
        builder.add_uniform_input(camera_buffer);
        builder.add_uniform_input(prev_camera);
        builder.add_inline_input(InlineDescriptor(settings_data));
        builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
            const auto* material = device_resources()[DeviceResources::TAAResolveMaterialTemplate];
            render_pass.bind_material_template(material, self->descriptor_sets());
            render_pass.draw_array(3);
        });

        pass.anti_aliased = aa;
    }

    return pass;
}

}

