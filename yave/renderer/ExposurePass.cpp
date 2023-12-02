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

#include "ExposurePass.h"

#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {

ExposurePass ExposurePass::create(FrameGraph& framegraph, FrameGraphImageId in_lit) {
    const auto region = framegraph.region("Exposure");

    static const math::Vec2ui histogram_size = math::Vec2ui(256, 1);
    const math::Vec2ui size = framegraph.image_size(in_lit);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Histogram gather pass");

    const auto histogram = builder.declare_image(VK_FORMAT_R32_UINT, histogram_size);

    builder.clear_before_pass(histogram);

    builder.add_storage_output(histogram);
    builder.add_uniform_input(in_lit);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const auto& program = device_resources()[DeviceResources::HistogramProgram];
        const u32 thread_count = program.local_size().x();
        y_debug_assert(thread_count == program.thread_count());
        math::Vec3ui groups(size / thread_count, 1);
        for(usize i = 0; i != 2; ++i) {
            groups[i] += groups[i] * thread_count < size[i] ? 1 : 0;
        }
        recorder.dispatch(program, groups, self->descriptor_sets());
    });

    FrameGraphComputePassBuilder params_builder = framegraph.add_compute_pass("Exposure compute pass");

    const auto params = params_builder.declare_typed_buffer<uniform::ExposureParams>(1);

    params_builder.add_storage_output(params);
    params_builder.add_uniform_input(histogram);
    params_builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const auto& program = device_resources()[DeviceResources::ExposureParamsProgram];
        recorder.dispatch(program, math::Vec3ui(1), self->descriptor_sets());
        y_debug_assert(program.thread_count() == histogram_size.x());
    });

    ExposurePass pass;
    pass.histogram = histogram;
    pass.params = params;

    return pass;
}

}

