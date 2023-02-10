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

#include "ToneMappingPass.h"

#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {

ToneMappingPass ToneMappingPass::create(FrameGraph& framegraph, FrameGraphImageId in_lit, const ExposurePass& exposure, const ToneMappingSettings& settings) {
    static constexpr ImageFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    const math::Vec2ui size = framegraph.image_size(in_lit);

    struct ShaderSettings {
        float exposure;
        u32 tone_mapper;
    } shader_settings{settings.exposure, u32(settings.tone_mapper)};

    FrameGraphPassBuilder builder = framegraph.add_pass("Tone mapping pass");

    const auto tone_mapped = builder.declare_image(format, size);

    auto params = exposure.params;

    FrameGraphMutableTypedBufferId<uniform::ExposureParams> mut_params;
    if(!settings.auto_exposure) {
        mut_params = builder.declare_typed_buffer<uniform::ExposureParams>();
        builder.map_buffer(mut_params);
        params = mut_params;
    }

    builder.add_color_output(tone_mapped);
    builder.add_uniform_input(in_lit);
    builder.add_uniform_input(params);
    builder.add_inline_input(InlineDescriptor(shader_settings), 0);
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        if(!settings.auto_exposure) {
            TypedMapping<uniform::ExposureParams> mapping = self->resources().map_buffer(mut_params);
            mapping[0] = uniform::ExposureParams{};
        }

        const auto* material = device_resources()[DeviceResources::ToneMappingMaterialTemplate];
        render_pass.bind_material_template(material, self->descriptor_sets()[0]);
        render_pass.draw_array(3);
    });


    ToneMappingPass pass;
    pass.tone_mapped = tone_mapped;
    return pass;
}

}

