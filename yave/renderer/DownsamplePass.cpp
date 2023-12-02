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

#include "DownsamplePass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <y/utils/format.h>

namespace yave {

static DeviceResources::MaterialTemplates material_template(DownsamplePass::Filter filter) {
    switch(filter) {
        case DownsamplePass::Filter::BestMatch:
            return DeviceResources::DownsampleMaterialTemplate;

        default:
            return DeviceResources::ScreenPassthroughMaterialTemplate;
    }
}

DownsamplePass DownsamplePass::create(FrameGraph& framegraph, FrameGraphImageId orig, usize mip_count, Filter filter, bool round_up) {
    DownsamplePass pass;
    pass.mips << orig;

    if(!mip_count) {
        return pass;
    }

    const auto region = framegraph.region("Downsample");

    const ImageFormat format = framegraph.image_format(orig).non_depth();
    math::Vec2ui mip_size = framegraph.image_size(orig);

    for(usize m = 1; pass.mips.size() < mip_count; ++m) {
        mip_size = (round_up ? (mip_size + 1) : mip_size) / 2;
        if(!mip_size.x() || !mip_size.y()) {
            break;
        }

        FrameGraphPassBuilder builder = framegraph.add_pass(fmt("Downsample pass x{}", 1 << m));

        const auto mip = builder.declare_image(format, mip_size);

        builder.add_color_output(mip);
        builder.add_uniform_input(pass.mips.last());
        builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
            const auto* material = device_resources()[material_template(filter)];
            render_pass.bind_material_template(material, self->descriptor_sets());
            render_pass.draw_array(3);
        });

        pass.mips << mip;
    }

    return pass;
}

}

