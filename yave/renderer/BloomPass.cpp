/*******************************
Copyright (c) 2016-2021 Gr�goire Angerand

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


namespace yave {

static FrameGraphImageId threshold(FrameGraph& framegraph, FrameGraphImageId input, const BloomSettings& settings) {
    const math::Vec2ui size = framegraph.image_size(input);
    const ImageFormat format = framegraph.image_format(input);
    const math::Vec2ui internal_size = /*size / 2*/ size;

    FrameGraphPassBuilder builder = framegraph.add_pass("Bloom pass");

    struct BloomParams {
        float power;
        float threshold;
        float rev_threshold;
    } params {
        settings.bloom_power,
        settings.bloom_threshold,
        settings.bloom_threshold >= 1.0f
            ? 0.0f
            : 1.0f / (1.0f - settings.bloom_threshold)
    };

    const auto thresholded = builder.declare_image(format, internal_size);

    builder.add_color_output(thresholded);
    builder.add_uniform_input(input);
    builder.add_inline_input(params);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        auto render_pass = recorder.bind_framebuffer(self->framebuffer());
        const auto* material = device_resources(recorder.device())[DeviceResources::BloomMaterialTemplate];
        render_pass.bind_material(material, {self->descriptor_sets()[0]});
        render_pass.draw_array(3);
    });

    return thresholded;
}

BloomPass BloomPass::create(FrameGraph& framegraph, FrameGraphImageId input, const BloomSettings& settings) {
    const auto region = framegraph.region("Bloom");

    const FrameGraphImageId thresholded = threshold(framegraph, input, settings);
    const DownsamplePass downsampled = DownsamplePass::create(framegraph, thresholded, settings.downsample_mips);
    const BlurPass blur = BlurPass::create(framegraph, downsampled.mips.last(), settings.blur);

    FrameGraphPassBuilder builder = framegraph.add_pass("Bloom merge pass");

    const auto merged = builder.declare_copy(input);

    builder.add_color_output(merged);
    builder.add_uniform_input(blur.blurred);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        auto render_pass = recorder.bind_framebuffer(self->framebuffer());
        const auto* material = device_resources(recorder.device())[DeviceResources::ScreenBlendPassthroughMaterialTemplate];
        render_pass.bind_material(material, {self->descriptor_sets()[0]});
        render_pass.draw_array(3);
    });

    BloomPass pass;
    pass.merged = merged;
    return pass;
}

}

