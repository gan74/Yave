/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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

#include "CmdBufferRecorder.h"

namespace yave {

CmdBufferRecorderBase::CmdBufferRecorderBase(vk::CommandBuffer buffer, CmdBufferUsage usage) : _cmd_buffer(buffer), _render_pass(nullptr){
	vk_cmd_buffer().begin(vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits(usage))
		);
}

void CmdBufferRecorderBase::swap(CmdBufferRecorderBase& other) {
	std::swap(_cmd_buffer, other._cmd_buffer);
	std::swap(_render_pass, other._render_pass);
}

vk::CommandBuffer CmdBufferRecorderBase::vk_cmd_buffer() const {
	return _cmd_buffer;
}

const RenderPass& CmdBufferRecorderBase::current_pass() const {
	return *_render_pass;
}

const Viewport& CmdBufferRecorderBase::viewport() const {
	return _viewport;
}

void CmdBufferRecorderBase::end_render_pass() {
	if(_render_pass) {
		vk_cmd_buffer().endRenderPass();
		_render_pass = nullptr;
	}
}

void CmdBufferRecorderBase::set_viewport(const Viewport& view) {
	_viewport = view;
}

void CmdBufferRecorderBase::bind_framebuffer(const Framebuffer& framebuffer) {
	if(_render_pass) {
		end_render_pass();
	}
	auto clear_values = core::vector_with_capacity<vk::ClearValue>(framebuffer.attachment_count() + 1);
	for(usize i = 0; i != framebuffer.attachment_count(); ++i) {
		clear_values << vk::ClearColorValue(std::array<float, 4>{{0.0f, 0.0f, 0.0f, 0.0f}});
	}
	clear_values << vk::ClearDepthStencilValue(1.0f, 0);

	auto pass_info = vk::RenderPassBeginInfo()
			.setRenderArea(vk::Rect2D({0, 0}, {framebuffer.size().x(), framebuffer.size().y()}))
			.setRenderPass(framebuffer.render_pass().vk_render_pass())
			.setFramebuffer(framebuffer.vk_framebuffer())
			.setPClearValues(clear_values.begin())
			.setClearValueCount(u32(clear_values.size()))
		;
	vk_cmd_buffer().beginRenderPass(pass_info, vk::SubpassContents::eInline);
	_render_pass = &framebuffer.render_pass();

	set_viewport(Viewport(framebuffer.size()));
}

void CmdBufferRecorderBase::bind_pipeline(const GraphicPipeline& pipeline, std::initializer_list<std::reference_wrapper<const DescriptorSet>> descriptor_sets) {
	vk_cmd_buffer().bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.vk_pipeline());

	auto ds = core::vector_with_capacity<vk::DescriptorSet>(descriptor_sets.size() + 1);
	std::transform(descriptor_sets.begin(), descriptor_sets.end(), std::back_inserter(ds), [](const auto& ds) { return ds.get().vk_descriptor_set(); });
	ds << pipeline.vk_descriptor_set();

	vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.vk_pipeline_layout(), 0, ds.size(), ds.begin(), 0, nullptr);
}

void CmdBufferRecorderBase::draw(const StaticMeshInstance& mesh_instance) {
	vk::DeviceSize offset = 0; // fohkin' vk::ArrayProxy

	vk_cmd_buffer().bindVertexBuffers(0, mesh_instance.vertex_buffer.vk_buffer(), offset);
	vk_cmd_buffer().bindIndexBuffer(mesh_instance.triangle_buffer.vk_buffer(), offset, vk::IndexType::eUint32);

	auto cmds = u32(mesh_instance.indirect_buffer.size());
	vk_cmd_buffer().drawIndexedIndirect(mesh_instance.indirect_buffer.vk_buffer(), offset, cmds, cmds ? sizeof(vk::DrawIndexedIndirectCommand) : 0);
}


void CmdBufferRecorderBase::dispatch(const ComputeProgram& program, const math::Vec3ui& size, std::initializer_list<std::reference_wrapper<const DescriptorSet>> descriptor_sets) {
	end_render_pass();

	auto ds = core::vector_with_capacity<vk::DescriptorSet>(descriptor_sets.size());
	std::transform(descriptor_sets.begin(), descriptor_sets.end(), std::back_inserter(ds), [](const auto& ds) { return ds.get().vk_descriptor_set(); });

	vk_cmd_buffer().bindPipeline(vk::PipelineBindPoint::eCompute, program.vk_pipeline());
	vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eCompute, program.vk_pipeline_layout(), 0, ds.size(), ds.begin(), 0, nullptr);
	vk_cmd_buffer().dispatch(size.x(), size.y(), size.z());
}

void CmdBufferRecorderBase::barriers(const core::ArrayProxy<BufferBarrier>& buffers, const core::ArrayProxy<ImageBarrier>& images, PipelineStage src, PipelineStage dst) {
	end_render_pass();

	auto image_barriers = core::vector_with_capacity<vk::ImageMemoryBarrier>(images.size());
	std::transform(images.begin(), images.end(), std::back_inserter(image_barriers), [](const auto& b) { return b.vk_barrier(); });

	auto buffer_barriers = core::vector_with_capacity<vk::BufferMemoryBarrier>(buffers.size());
	std::transform(buffers.begin(), buffers.end(), std::back_inserter(buffer_barriers), [](const auto& b) { return b.vk_barrier(); });

	vk_cmd_buffer().pipelineBarrier(
			vk::PipelineStageFlagBits(src),
			vk::PipelineStageFlagBits(dst),
			vk::DependencyFlagBits::eByRegion,
			0, nullptr,
			buffer_barriers.size(), buffer_barriers.begin(),
			image_barriers.size(), image_barriers.begin()
		);
}

void CmdBufferRecorderBase::transition_image(ImageBase& image, vk::ImageLayout src, vk::ImageLayout dst) {
	auto barrier = create_image_barrier(
			image.vk_image(),
			image.format(),
			src,
			dst
		);

	vk_cmd_buffer().pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::DependencyFlagBits::eByRegion,
			nullptr, nullptr, barrier
		);
}

}
