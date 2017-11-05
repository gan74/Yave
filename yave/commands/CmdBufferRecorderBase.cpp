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
#include <yave/material/Material.h>

namespace yave {

static vk::CommandBufferUsageFlagBits cmd_usage(CmdBufferUsage u) {
	return vk::CommandBufferUsageFlagBits(uenum(u) & ~uenum(CmdBufferUsage::Secondary));
}

CmdBufferRecorderBase::CmdBufferRecorderBase(CmdBufferBase&& base) : _cmd_buffer(std::move(base)) {
}

CmdBufferRecorderBase::~CmdBufferRecorderBase() {
	if(vk_cmd_buffer()) {
		fatal("CmdBufferRecorderBase<> destroyed before end() was called.");
	}
}

void CmdBufferRecorderBase::swap(CmdBufferRecorderBase& other) {
	_cmd_buffer.swap(other._cmd_buffer);
	std::swap(_render_pass, other._render_pass);
}

vk::CommandBuffer CmdBufferRecorderBase::vk_cmd_buffer() const {
	return _cmd_buffer.vk_cmd_buffer();
}

const RenderPass& CmdBufferRecorderBase::current_pass() const {
	if(!_render_pass) {
		fatal("Null renderpass.");
	}
	return *_render_pass;
}

void CmdBufferRecorderBase::set_viewport(const Viewport& view) {
	vk_cmd_buffer().setViewport(0, {vk::Viewport(view.offset.x(), view.offset.y(), view.extent.x(), view.extent.y(), view.depth.x(), view.depth.y())});
	vk_cmd_buffer().setScissor(0, {vk::Rect2D(vk::Offset2D(view.offset.x(), view.offset.y()), vk::Extent2D(view.extent.x(), view.extent.y()))});
}

void CmdBufferRecorderBase::bind_material(const Material& material, DescriptorSetList descriptor_sets) {
	bind_pipeline(material.compile(current_pass()), descriptor_sets);
}

void CmdBufferRecorderBase::bind_pipeline(const GraphicPipeline& pipeline, DescriptorSetList descriptor_sets) {
	vk_cmd_buffer().bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.vk_pipeline());

	auto ds = core::vector_with_capacity<vk::DescriptorSet>(descriptor_sets.size() + 1);
	std::transform(descriptor_sets.begin(), descriptor_sets.end(), std::back_inserter(ds), [](const auto& ds) { return ds.get().vk_descriptor_set(); });
	if(pipeline.vk_descriptor_set()) {
		ds << pipeline.vk_descriptor_set();
	}

	vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.vk_pipeline_layout(), 0, vk::ArrayProxy(u32(ds.size()), ds.cbegin()), {});
}


void CmdBufferRecorderBase::draw(const vk::DrawIndexedIndirectCommand& indirect) {
	vk_cmd_buffer().drawIndexed(indirect.indexCount,
								indirect.instanceCount,
								indirect.firstIndex,
								indirect.vertexOffset,
								indirect.firstInstance);
}

void CmdBufferRecorderBase::draw(usize vertices) {
	vk_cmd_buffer().draw(u32(vertices), 1, 0, 0);
}

void CmdBufferRecorderBase::bind_buffers(const SubBuffer<BufferUsage::IndexBit>& indices, const core::ArrayView<SubBuffer<BufferUsage::AttributeBit>>& attribs) {
	bind_index_buffer(indices);
	bind_attrib_buffers(attribs);
}

void CmdBufferRecorderBase::bind_index_buffer(const SubBuffer<BufferUsage::IndexBit>& indices) {
	vk_cmd_buffer().bindIndexBuffer(indices.vk_buffer(), indices.byte_offset(), vk::IndexType::eUint32);
}

void CmdBufferRecorderBase::bind_attrib_buffers(const core::ArrayView<SubBuffer<BufferUsage::AttributeBit>>& attribs) {
	u32 attrib_count = attribs.size();

	auto offsets = core::vector_with_capacity<vk::DeviceSize>(attrib_count);
	auto buffers = core::vector_with_capacity<vk::Buffer>(attrib_count);
	std::transform(attribs.begin(), attribs.end(), std::back_inserter(offsets), [](const auto& buffer) { return buffer.byte_offset(); });
	std::transform(attribs.begin(), attribs.end(), std::back_inserter(buffers), [](const auto& buffer) { return buffer.vk_buffer(); });

	vk_cmd_buffer().bindVertexBuffers(u32(0), vk::ArrayProxy(attrib_count, buffers.cbegin()), vk::ArrayProxy(attrib_count, offsets.cbegin()));
}


SecondaryCmdBufferRecorderBase::SecondaryCmdBufferRecorderBase(CmdBufferBase&& base, const Framebuffer& framebuffer) :
			CmdBufferRecorderBase(std::move(base)) {

	_render_pass = &framebuffer.render_pass();

	auto inherit_info = vk::CommandBufferInheritanceInfo()
			.setFramebuffer(framebuffer.vk_framebuffer())
			.setRenderPass(framebuffer.render_pass().vk_render_pass())
		;

	auto info = vk::CommandBufferBeginInfo()
			.setFlags(cmd_usage(CmdBufferUsage::Secondary) | vk::CommandBufferUsageFlagBits::eRenderPassContinue)
			.setPInheritanceInfo(&inherit_info)
		;

	vk_cmd_buffer().begin(info);
	set_viewport(Viewport(framebuffer.size()));
}






PrimaryCmdBufferRecorderBase::PrimaryCmdBufferRecorderBase(CmdBufferBase&& base, CmdBufferUsage usage) :
		CmdBufferRecorderBase(std::move(base)) {

	auto info = vk::CommandBufferBeginInfo()
			.setFlags(cmd_usage(usage))
		;

	vk_cmd_buffer().begin(info);
}

void PrimaryCmdBufferRecorderBase::end_renderpass() {
	if(_render_pass) {
		vk_cmd_buffer().endRenderPass();
		_render_pass = nullptr;
	}
}

void PrimaryCmdBufferRecorderBase::execute(const RecordedCmdBuffer<CmdBufferUsage::Secondary>& secondary, const Framebuffer& framebuffer) {
	_cmd_buffer.keep_alive(secondary._proxy);

	bind_framebuffer(framebuffer, vk::SubpassContents::eSecondaryCommandBuffers);
	vk_cmd_buffer().executeCommands({secondary.vk_cmd_buffer()});
	end_renderpass();
}


void PrimaryCmdBufferRecorderBase::dispatch(const ComputeProgram& program, const math::Vec3ui& size, DescriptorSetList descriptor_sets, const PushConstant& push_constants) {
	end_renderpass();

	auto ds = core::vector_with_capacity<vk::DescriptorSet>(descriptor_sets.size());
	std::transform(descriptor_sets.begin(), descriptor_sets.end(), std::back_inserter(ds), [](const auto& ds) { return ds.get().vk_descriptor_set(); });

	vk_cmd_buffer().bindPipeline(vk::PipelineBindPoint::eCompute, program.vk_pipeline());

	if(ds.size()) {
		vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eCompute, program.vk_pipeline_layout(), 0, ds.size(), ds.begin(), 0, nullptr);
	}

	if(push_constants.size()) {
		vk_cmd_buffer().pushConstants(program.vk_pipeline_layout(), vk::ShaderStageFlagBits::eCompute, 0, push_constants.size(), push_constants.data());
	}

	vk_cmd_buffer().dispatch(size.x(), size.y(), size.z());
}


void PrimaryCmdBufferRecorderBase::dispatch_size(const ComputeProgram& program, const math::Vec3ui& size, DescriptorSetList descriptor_sets, const PushConstant& push_constants) {
	math::Vec3ui dispatch_size;
	for(usize i = 0; i != 3; ++i) {
		dispatch_size[i] = size[i] / program.local_size()[i] + !!(size[i] % program.local_size()[i]);
	}
	dispatch(program, dispatch_size, descriptor_sets, push_constants);
}

void PrimaryCmdBufferRecorderBase::dispatch_size(const ComputeProgram& program, const math::Vec2ui& size, DescriptorSetList descriptor_sets, const PushConstant& push_constants) {
	dispatch_size(program, math::Vec3ui(size, 1), descriptor_sets, push_constants);
}

void PrimaryCmdBufferRecorderBase::barriers(const core::ArrayView<BufferBarrier>& buffers, const core::ArrayView<ImageBarrier>& images, PipelineStage src, PipelineStage dst) {
	end_renderpass();

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

void PrimaryCmdBufferRecorderBase::bind_framebuffer(const Framebuffer& framebuffer) {
	bind_framebuffer(framebuffer, vk::SubpassContents::eInline);
	set_viewport(Viewport(framebuffer.size()));
}


static vk::PipelineStageFlags pipeline_stage(vk::AccessFlags access) {
	if((uenum(access) & ~uenum(vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite)) == 0) {
		return vk::PipelineStageFlagBits::eTopOfPipe;
	}
	if(access & (vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite)) {
		return vk::PipelineStageFlagBits::eTransfer;
	}
	if(access & (vk::AccessFlagBits::eShaderRead| vk::AccessFlagBits::eShaderWrite)) {
		return vk::PipelineStageFlagBits::eVertexShader |
			   vk::PipelineStageFlagBits::eFragmentShader |
			   //vk::PipelineStageFlagBits::eTessellationControlShader |
			   //vk::PipelineStageFlagBits::eTessellationEvaluationShader |
			   vk::PipelineStageFlagBits::eComputeShader;
	}

	return fatal("Unknown access flags.");
}

void PrimaryCmdBufferRecorderBase::transition_image(ImageBase& image, vk::ImageLayout src, vk::ImageLayout dst) {
	auto barrier = create_image_barrier(
			image.vk_image(),
			image.format(),
			image.layers(),
			image.mipmaps(),
			src,
			dst
		);

	vk_cmd_buffer().pipelineBarrier(
			pipeline_stage(barrier.srcAccessMask),
			pipeline_stage(barrier.dstAccessMask),
			vk::DependencyFlagBits::eByRegion,
			nullptr, nullptr, barrier
		);
}

void PrimaryCmdBufferRecorderBase::bind_framebuffer(const Framebuffer& framebuffer, vk::SubpassContents subpass) {
	if(_render_pass) {
		end_renderpass();
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

	vk_cmd_buffer().beginRenderPass(pass_info, subpass);
	_render_pass = &framebuffer.render_pass();
}

}
