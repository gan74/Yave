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

#include "TemporalPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/graphics/device/DeviceResources.h>

namespace yave {

TemporalPass TemporalPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId in_color, FrameGraphPersistentResourceId persistent_id) {
    const math::Vec2ui size = framegraph.image_size(in_color);
    const ImageFormat format = framegraph.image_format(in_color);

    FrameGraphPassBuilder builder = framegraph.add_pass("Temporal pass");

    const auto out = builder.declare_image(format, size);
    builder.add_input_usage(out, ImageUsage::TextureBit);

    const FrameGraphImageId prev = framegraph.make_persistent_and_get_prev(out, persistent_id);
    if(!prev.is_valid()) {
        TemporalPass pass;
        pass.out = in_color;
        return pass;
    }

    builder.add_uniform_input(in_color);
    builder.add_uniform_input(prev);
    builder.add_uniform_input(gbuffer.motion);
    builder.add_color_output(out);
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        const auto* material = device_resources()[DeviceResources::TemporalMaterialTemplate];
        render_pass.bind_material_template(material, self->descriptor_sets());
        render_pass.draw_array(3);
    });

    TemporalPass pass;
    pass.out = out;
    return pass;
}

}

