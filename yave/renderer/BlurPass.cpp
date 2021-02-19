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

#include "BlurPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {

BlurPass BlurPass::create(FrameGraph& framegraph, FrameGraphImageId in_image) {
    const math::Vec2ui size = framegraph.image_size(in_image);
    const ImageFormat format = framegraph.image_format(in_image);

    auto blur_sub_pass = [&](FrameGraphPassBuilder builder, FrameGraphImageId in, DeviceResources::MaterialTemplates mat) -> FrameGraphMutableImageId {
        const auto blurred = builder.declare_image(format, size);

        builder.add_color_output(blurred);
        builder.add_uniform_input(in);
        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            auto render_pass = recorder.bind_framebuffer(self->framebuffer());
            const auto* material = device_resources(recorder.device())[mat];
            render_pass.bind_material(material, {self->descriptor_sets()[0]});
            render_pass.draw_array(3);
        });

        return blurred;
    };

    const FrameGraphMutableImageId h_blur = blur_sub_pass(framegraph.add_pass("Blur horizontal pass"), in_image, DeviceResources::HBlurMaterialTemplate);
    const FrameGraphMutableImageId v_blur = blur_sub_pass(framegraph.add_pass("Blur vertical pass"), h_blur, DeviceResources::VBlurMaterialTemplate);

    BlurPass pass;
    pass.blurred = v_blur;

    return pass;
}

}

