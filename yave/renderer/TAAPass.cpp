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

#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {

TAAPass TAAPass::create(FrameGraph& framegraph, FrameGraphImageId in) {
    static const FrameGraphPersistentResourceId persitent_id = FrameGraphPersistentResourceId::create();

    const ImageFormat format = framegraph.image_format(in);
    const math::Vec2ui size = framegraph.image_size(in);

    FrameGraphPassBuilder builder = framegraph.add_pass("TAA resolve pass");

    const auto aa = builder.declare_image(format, size);

    FrameGraphImageId prev = framegraph.make_persistent_and_get_prev(aa, persitent_id);
    if(prev == aa) {
        prev = builder.declare_copy(aa);
    }

    builder.add_image_input_usage(aa, ImageUsage::TextureBit);

    builder.add_color_output(aa);
    builder.add_uniform_input(in);
    builder.add_uniform_input(prev);
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

