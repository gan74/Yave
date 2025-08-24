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

#include "BilateralPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/framegraph/FrameGraphPassBuilder.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/graphics/device/DeviceResources.h>

namespace yave {

static FrameGraphImageId bilateral(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId in, bool horizontal, const BilateralSettings& settings) {
    const auto weights = math::compute_gaussian_weights<float, 64>(settings.sigma);

    const math::Vec2ui orig_size = framegraph.image_size(in);
    const math::Vec2ui target_size = framegraph.image_size(gbuffer.depth);
    const math::Vec2ui size(horizontal ? target_size.x() : orig_size.x(), horizontal ? orig_size.y() : target_size.y());
    const ImageFormat format = framegraph.image_format(in);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass(horizontal ? "Bilateral horizontal pass" : "Bilateral vertical pass");

    const auto filtered = builder.declare_image(format, size);

    struct Params {
        float depth_phi;
        float normal_phi;
        float padding[2];
    } params {
        settings.depth_phi,
        settings.normal_phi,
        {}
    };

    builder.add_uniform_input(in, SamplerType::LinearClamp);
    builder.add_uniform_input(gbuffer.depth, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.normal, SamplerType::PointClamp);
    builder.add_storage_output(filtered);
    builder.add_inline_input(weights);
    builder.add_inline_input(params);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        recorder.dispatch_threads(device_resources()[horizontal ? DeviceResources::BilateralHorizontalProgram : DeviceResources::BilateralVerticalProgram], size, self->descriptor_set());
    });

    return filtered;
}

BilateralPass BilateralPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId in, const BilateralSettings& settings) {
    BilateralPass pass;
    pass.filtered = bilateral(framegraph, gbuffer, bilateral(framegraph, gbuffer, in, false, settings), true, settings);
    return pass;
}

}

