/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include <yave/material/Material.h>

namespace yave {

ToneMappingPass ToneMappingPass::create(FrameGraph& framegraph, const LightingPass& lighting) {
	static constexpr vk::Format format = vk::Format::eR8G8B8A8Unorm;
	math::Vec2ui size = framegraph.image_size(lighting.lit);

	auto tone_mapped = framegraph.declare_image(format, size);

	ToneMappingPass pass;
	pass.tone_mapped = tone_mapped;

	FrameGraphPassBuilder builder = framegraph.add_pass("Tone mapping pass");
	builder.add_color_output(tone_mapped);
	builder.add_uniform_input(lighting.lit, 0, PipelineStage::FragmentBit);
	builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			auto render_pass = recorder.bind_framebuffer(self->framebuffer());
			const auto* material = recorder.device()->device_resources()[DeviceResources::TonemappingMaterialTemplate];
			render_pass.bind_material(material, {self->descriptor_sets()[0]});
			render_pass.draw(vk::DrawIndirectCommand(6, 1));
		});

	return pass;
}

}
