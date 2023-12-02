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

#include "BlurPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {

static constexpr usize sample_count = 8;

static std::array<float, sample_count> compute_gaussian_weights(float sigma) {
    const float denom = 2.0f * sigma * sigma;

    float total = 0.0f;
    std::array<float, sample_count> weights = {};
    for(usize i = 0; i != weights.size(); ++i) {
        const float w = std::exp(-float(i * i) / denom);
        weights[i] = w;
        total += i == 0 ? 2.0f * w : w;
    }
    for(usize i = 0; i != sample_count; ++i) {
        weights[i] /= total;
    }
    return weights;
}

static BlurPass create_blur(FrameGraph& framegraph, FrameGraphImageId in_image, const math::Vec2ui& in_size, const math::Vec2ui& out_size, const BlurSettings& settings) {
    const auto region = framegraph.region("Blur");

    const ImageFormat format = framegraph.image_format(in_image);

    const auto weights = compute_gaussian_weights(settings.sigma);

    auto blur_sub_pass = [&](FrameGraphPassBuilder builder, const math::Vec2ui& target_size, FrameGraphImageId in, DeviceResources::MaterialTemplates mat) -> FrameGraphMutableImageId {
        const auto blurred = builder.declare_image(format, target_size);

        builder.add_color_output(blurred);
        builder.add_uniform_input(in);
        builder.add_inline_input(InlineDescriptor(weights));
        builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
            const auto* material = device_resources()[mat];
            render_pass.bind_material_template(material, self->descriptor_sets());
            render_pass.draw_array(3);
        });

        return blurred;
    };

    const math::Vec2ui v_size(in_size.x(), out_size.y());

    const FrameGraphMutableImageId v_blur = blur_sub_pass(framegraph.add_pass("Blur vertical pass"), v_size, in_image, DeviceResources::VBlurMaterialTemplate);
    const FrameGraphMutableImageId h_blur = blur_sub_pass(framegraph.add_pass("Blur horizontal pass"), out_size, v_blur, DeviceResources::HBlurMaterialTemplate);

    BlurPass pass;
    pass.blurred = h_blur;
    return pass;
}


BlurPass BlurPass::create(FrameGraph& framegraph, FrameGraphImageId in_image, const BlurSettings& settings) {
    const math::Vec2ui in_size = framegraph.image_size(in_image);
    return create_blur(framegraph, in_image, in_size, in_size, settings);
}

BlurPass BlurPass::create(FrameGraph& framegraph, FrameGraphImageId in_image, const math::Vec2ui& size, const BlurSettings& settings) {
    const math::Vec2ui in_size = framegraph.image_size(in_image);
    return create_blur(framegraph, in_image, in_size, size, settings);
}

}

