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

#include "TAAPass.h"

#include "TemporalPass.h"

#include <yave/camera/Camera.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {

TAAPass TAAPass::create(FrameGraph& framegraph, const TemporalDesocclusionPass& temp, FrameGraphImageId in_color, const TAASettings& settings) {
    if(!settings.enable) {
        TAAPass pass;
        pass.anti_aliased = in_color;
        return pass;
    }

    const ImageFormat format = framegraph.image_format(in_color);
    const math::Vec2ui size = framegraph.image_size(in_color);

    y_always_assert(!temp.mask.is_valid() || framegraph.image_size(temp.mask) == size, "Invalid temporal mask size");

    FrameGraphPassBuilder builder = framegraph.add_pass("TAA resolve pass");

    const auto aa = builder.declare_image(format, size);
    builder.add_input_usage(aa, ImageUsage::TextureBit);

    static const FrameGraphPersistentResourceId color_id = FrameGraphPersistentResourceId::create();
    FrameGraphImageId prev_color = framegraph.make_persistent_and_get_prev(aa, color_id);
    if(!prev_color.is_valid()) {
        prev_color = builder.declare_copy(aa);
    }

    const Texture& zero = *device_resources()[DeviceResources::ZeroTexture];

    const u32 clamping_bit                  = 0x01;
    const u32 match_prev_bitmatch_prev_bit  = 0x02;
    const u32 weighted_clamp_bit            = 0x04;

    u32 flag_bits = 0;
    flag_bits |= (settings.use_clamping ? clamping_bit : 0);
    flag_bits |= (settings.use_previous_matching ? match_prev_bitmatch_prev_bit : 0);
    flag_bits |= (settings.use_weighted_clamp ? weighted_clamp_bit : 0);

    struct SettingsData {
        u32 flags;
        u32 weighting_mode;
        float blending_factor;
    } settings_data {
        flag_bits,
        u32(settings.weighting_mode),
        settings.blending_factor,
    };

    builder.add_uniform_input(in_color, SamplerType::PointClamp);
    builder.add_uniform_input(prev_color);
    builder.add_uniform_input(temp.motion);
    builder.add_uniform_input_with_default(temp.mask, Descriptor(zero, SamplerType::PointClamp));
    builder.add_uniform_input(temp.camera);
    builder.add_inline_input(InlineDescriptor(settings_data));
    builder.add_color_output(aa);
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

