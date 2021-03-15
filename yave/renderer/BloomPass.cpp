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

static FrameGraphImageId threshold(FrameGraph& framegraph, FrameGraphImageId input, const math::Vec2ui& size, const BloomSettings& settings) {
    const ImageFormat format = framegraph.image_format(input);

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

    const auto thresholded = builder.declare_image(format, size);

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

    const math::Vec2ui size = framegraph.image_size(input);
    const FrameGraphImageId thresholded = threshold(framegraph, input, size, settings);

    const usize pyramid_count = std::max(settings.pyramids, usize(1));
    auto pyramids = core::vector_with_capacity<FrameGraphImageId>(pyramid_count);

    {
        const auto region = framegraph.region("Pyramid downsample");
        FrameGraphImageId src = thresholded;

        for(usize i = 0; i != pyramid_count; ++i) {
            const math::Vec2ui pyramid_size(size.x() >> (i + 1), size.y() >> (i + 1));
            if(!pyramid_size.x() || !pyramid_size.y()) {
                break;
            }
            src = pyramids.emplace_back(BlurPass::create(framegraph, src, pyramid_size, settings.blur).blurred);
        }
    }

    FrameGraphImageId bloomed = input;
    if(!pyramids.is_empty()) {
        const auto region = framegraph.region("Pryramid merge");

        bloomed = pyramids.last();
        auto merge = [&](FrameGraphPassBuilder builder,  FrameGraphImageId dst) {
            const auto merged = builder.declare_copy(dst);

            builder.add_color_output(merged);
            builder.add_uniform_input(bloomed);
            builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
                auto render_pass = recorder.bind_framebuffer(self->framebuffer());
                const auto* material = device_resources(recorder.device())[DeviceResources::ScreenBlendPassthroughMaterialTemplate];
                render_pass.bind_material(material, {self->descriptor_sets()[0]});
                render_pass.draw_array(3);
            });

            bloomed = merged;
        };

        for(usize i = 1; i < pyramids.size(); ++i) {
            merge(framegraph.add_pass("Merge pass"), pyramids[pyramids.size() - i - 1]);
        }
        merge(framegraph.add_pass("Final merge pass"), input);
    }

    y_debug_assert(framegraph.image_size(bloomed) == size);

    BloomPass pass;
    pass.bloomed = bloomed;
    return pass;
}

}

