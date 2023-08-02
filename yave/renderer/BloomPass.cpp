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

#include "BloomPass.h"
#include "ToneMappingPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <y/core/ScratchPad.h>

namespace yave {

BloomPass BloomPass::create(FrameGraph& framegraph, FrameGraphImageId input, FrameGraphTypedBufferId<uniform::ExposureParams> exposure, const BloomSettings& settings) {
    const auto region = framegraph.region("Bloom");

    if(settings.intensity <= 0.0f) {
        BloomPass pass;
        pass.bloomed = input;
        return pass;
    }

    const math::Vec2ui orig_size = framegraph.image_size(input);
    const ImageFormat format = framegraph.image_format(input);

    const float ratio = orig_size.x() / float(orig_size.y());
    const math::Vec2 filter_size = math::Vec2(1.0f, ratio) * settings.radius;

    core::ScratchVector<FrameGraphImageId> mips(settings.pyramids + 1);
    mips.emplace_back(input);

    for(usize i = 1; i != settings.pyramids + 1; ++i) {
        const math::Vec2ui size = math::Vec2ui(orig_size.x() >> i, orig_size.y() >> i);
        if(!size.x() || !size.y()) {
            break;
        }

        FrameGraphPassBuilder builder = framegraph.add_pass("Bloom downsample pass");

        const auto downscaled = builder.declare_image(format, size);

        builder.add_color_output(downscaled);
        builder.add_uniform_input(mips.last(), SamplerType::LinearClamp);
        builder.add_uniform_input(exposure);
        builder.add_inline_input(InlineDescriptor(u32(i - 1)));
        builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
            const auto* material = device_resources()[DeviceResources::BloomDownscaleMaterialTemplate];
            render_pass.bind_material_template(material, self->descriptor_sets());
            render_pass.draw_array(3);
        });

        mips.emplace_back(downscaled);
    }

    for(usize i = mips.size() - 1; i != 0; --i) {
        FrameGraphPassBuilder builder = framegraph.add_pass("Bloom upsample pass");

        const bool final_pass = i == 1;
        const math::Vec3 params = math::Vec3(filter_size, final_pass ? settings.intensity : 1.0f);

        const auto upscaled = builder.declare_copy(mips[i - 1]);

        builder.add_color_output(upscaled);
        builder.add_uniform_input(mips[i], SamplerType::LinearClamp);
        builder.add_inline_input(InlineDescriptor(params));
        builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
            const auto* material = device_resources()[DeviceResources::BloomUpscaleMaterialTemplate];
            render_pass.bind_material_template(material, self->descriptor_sets());
            render_pass.draw_array(3);
        });

        mips[i - 1] = upscaled;
    }


    BloomPass pass;
    pass.bloomed = mips[0];
    return pass;
}

}

