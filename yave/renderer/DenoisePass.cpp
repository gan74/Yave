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

#include "DenoisePass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {


DenoisePass DenoisePass::create(FrameGraph& framegraph, FrameGraphImageId in_image, const DenoiseSettings& settings) {
    const math::Vec2ui size = framegraph.image_size(in_image);
    const ImageFormat format = framegraph.image_format(in_image);

    FrameGraphPassBuilder builder = framegraph.add_pass("Denoise pass");

    const auto denoised = builder.declare_image(format, size);

    builder.add_color_output(denoised);
    builder.add_uniform_input(in_image);
    builder.add_inline_input(InlineDescriptor(settings));
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        const auto* material = device_resources()[DeviceResources::DenoiseMaterialTemplate];
        render_pass.bind_material_template(material, self->descriptor_set());
        render_pass.draw_array(3);
    });

    DenoisePass pass;
    pass.denoised = denoised;
    return pass;
}
}

