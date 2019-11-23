/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#include <yave/material/Material.h>
#include <yave/framegraph/FrameGraph.h>

#include <y/core/Chrono.h>

namespace yave {

ToneMappingPass ToneMappingPass::create(FrameGraph& framegraph, FrameGraphImageId in_lit, const ToneMappingSettings& settings) {
	static constexpr vk::Format format = vk::Format::eR8G8B8A8Unorm;
	static const math::Vec2ui histogram_size = math::Vec2ui(256, 1);

	const math::Vec2ui size = framegraph.image_size(in_lit);

	FrameGraphMutableImageId histogram;
	FrameGraphMutableTypedBufferId<uniform::ToneMappingParams> params;

	if(settings.auto_exposure) {
		FrameGraphPassBuilder clear_builder = framegraph.add_pass("Histogram clear pass");

		histogram = clear_builder.declare_image(vk::Format::eR32Uint, histogram_size);

		clear_builder.add_storage_output(histogram, 0, PipelineStage::ComputeBit);
		clear_builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			const auto& program = recorder.device()->device_resources()[DeviceResources::HistogramClearProgram];
			recorder.dispatch_size(program, histogram_size, {self->descriptor_sets()[0]});
		});

		FrameGraphPassBuilder histogram_builder = framegraph.add_pass("Histogram compute pass");

		histogram_builder.add_storage_output(histogram, 0, PipelineStage::ComputeBit);
		histogram_builder.add_uniform_input(in_lit, 0, PipelineStage::ComputeBit);
		histogram_builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			const auto& program = recorder.device()->device_resources()[DeviceResources::HistogramProgram];
			recorder.dispatch_size(program, size, {self->descriptor_sets()[0]});
			y_debug_assert(program.thread_count() == histogram_size.x());
		});

		FrameGraphPassBuilder params_builder = framegraph.add_pass("Tone mapping params pass");

		params = params_builder.declare_typed_buffer<uniform::ToneMappingParams>(1);

		params_builder.add_storage_output(params, 0, PipelineStage::ComputeBit);
		params_builder.add_uniform_input(histogram, 0, PipelineStage::ComputeBit);
		params_builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			const auto& program = recorder.device()->device_resources()[DeviceResources::ToneMapParamsProgram];
			recorder.dispatch(program, math::Vec3ui(1), {self->descriptor_sets()[0]});
			y_debug_assert(program.thread_count() == histogram_size.x());
		});
	}

	FrameGraphPassBuilder builder = framegraph.add_pass("Tone mapping pass");

	const auto tone_mapped = builder.declare_image(format, size);
	const auto key_value = builder.declare_typed_buffer<float>();

	if(!settings.auto_exposure) {
		params = builder.declare_typed_buffer<uniform::ToneMappingParams>();
		builder.map_update(params);
	}

	builder.add_color_output(tone_mapped);
	builder.add_uniform_input(in_lit, 0, PipelineStage::FragmentBit);
	builder.add_uniform_input(params, 0, PipelineStage::FragmentBit);
	builder.add_uniform_input(key_value, 0, PipelineStage::FragmentBit);
	builder.map_update(key_value);
	builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
		self->resources().mapped_buffer(key_value)[0] = settings.key_value;
		if(!settings.auto_exposure) {
			TypedMapping<uniform::ToneMappingParams> mapping = self->resources().mapped_buffer(params);
			mapping[0] = uniform::ToneMappingParams();
		}


		auto render_pass = recorder.bind_framebuffer(self->framebuffer());
		const auto* material = recorder.device()->device_resources()[DeviceResources::ToneMappingMaterialTemplate];
		render_pass.bind_material(material, {self->descriptor_sets()[0]});
		render_pass.draw(vk::DrawIndirectCommand(6, 1));
	});


	ToneMappingPass pass;
	pass.tone_mapped = tone_mapped;
	pass.histogram = histogram;
	pass.params = params;

	return pass;
}

}
