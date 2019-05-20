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

#include "CmdBufferRecorder.h"

#include <yave/material/Material.h>
#include <yave/graphics/bindings/DescriptorSet.h>
#include <yave/graphics/framebuffer/Framebuffer.h>
#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/graphics/queues/Semaphore.h>

namespace yave {

static vk::CommandBufferUsageFlagBits cmd_usage(CmdBufferUsage u) {
	return vk::CommandBufferUsageFlagBits(uenum(u) /*& ~uenum(CmdBufferUsage::Secondary)*/);
}

static vk::PipelineStageFlags pipeline_stage(vk::AccessFlags access) {
	if(access == vk::AccessFlags() || access == vk::AccessFlagBits::eMemoryRead) {
		return vk::PipelineStageFlagBits::eHost;
	}
	if(access & (vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentRead)) {
		return vk::PipelineStageFlagBits::eEarlyFragmentTests |
			   vk::PipelineStageFlagBits::eLateFragmentTests;
	}
	if(access & (vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentRead)) {
		return vk::PipelineStageFlagBits::eColorAttachmentOutput;
	}
	if(access & (vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite)) {
		return vk::PipelineStageFlagBits::eTransfer;
	}
	if(access & (vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite)) {
		return vk::PipelineStageFlagBits::eVertexShader |
			   vk::PipelineStageFlagBits::eFragmentShader |
			   vk::PipelineStageFlagBits::eComputeShader;
	}

	return y_fatal("Unknown access flags.");
}


// -------------------------------------------------- CmdBufferRegion --------------------------------------------------

CmdBufferRegion::~CmdBufferRegion() {
	if(device() && device()->debug_marker()) {
		device()->debug_marker()->end_region(_buffer);
	}
}

CmdBufferRegion::CmdBufferRegion(const CmdBufferRecorder& cmd_buffer, const char* name, const math::Vec4& color) :
		DeviceLinked(cmd_buffer.device()),
		_buffer(cmd_buffer.vk_cmd_buffer()) {

	if(auto marker = device()->debug_marker()) {
		marker->begin_region(_buffer, name, color);
	}
}


// -------------------------------------------------- RenderPassRecorder --------------------------------------------------

RenderPassRecorder::RenderPassRecorder(CmdBufferRecorder& cmd_buffer, const Viewport& viewport) : _cmd_buffer(cmd_buffer), _viewport(viewport) {
}

RenderPassRecorder::~RenderPassRecorder() {
	_cmd_buffer.end_renderpass();
}

void RenderPassRecorder::bind_material(const Material& material) {
	bind_material(material.mat_template(), {material.descriptor_set()});
}

void RenderPassRecorder::bind_material(const MaterialTemplate* material, DescriptorSetList descriptor_sets) {
	bind_pipeline(material->compile(*_cmd_buffer._render_pass), descriptor_sets);
}

void RenderPassRecorder::bind_pipeline(const GraphicPipeline& pipeline, DescriptorSetList descriptor_sets) {
	vk_cmd_buffer().bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.vk_pipeline());

	auto ds = core::vector_with_capacity<vk::DescriptorSet>(descriptor_sets.size() + 1);
	std::transform(descriptor_sets.begin(), descriptor_sets.end(), std::back_inserter(ds), [](const auto& d) { return d.get().vk_descriptor_set(); });

	if(!ds.is_empty()) {
		vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.vk_pipeline_layout(), 0, vk::ArrayProxy(u32(ds.size()), ds.cbegin()), {});
	}
}


void RenderPassRecorder::draw(const vk::DrawIndexedIndirectCommand& indirect) {
	vk_cmd_buffer().drawIndexed(indirect.indexCount,
								indirect.instanceCount,
								indirect.firstIndex,
								indirect.vertexOffset,
								indirect.firstInstance);
}

void RenderPassRecorder::draw(const vk::DrawIndirectCommand& indirect) {
	vk_cmd_buffer().draw(indirect.vertexCount,
						 indirect.instanceCount,
						 indirect.firstVertex,
						 indirect.firstInstance);
}

void RenderPassRecorder::bind_buffers(const SubBuffer<BufferUsage::IndexBit>& indices, const core::ArrayView<SubBuffer<BufferUsage::AttributeBit>>& attribs) {
	bind_index_buffer(indices);
	bind_attrib_buffers(attribs);
}

void RenderPassRecorder::bind_index_buffer(const SubBuffer<BufferUsage::IndexBit>& indices) {
	vk_cmd_buffer().bindIndexBuffer(indices.vk_buffer(), indices.byte_offset(), vk::IndexType::eUint32);
}

void RenderPassRecorder::bind_attrib_buffers(const core::ArrayView<SubBuffer<BufferUsage::AttributeBit>>& attribs) {
	u32 attrib_count = attribs.size();

	auto offsets = core::vector_with_capacity<vk::DeviceSize>(attrib_count);
	auto buffers = core::vector_with_capacity<vk::Buffer>(attrib_count);
	std::transform(attribs.begin(), attribs.end(), std::back_inserter(offsets), [](const auto& buffer) { return buffer.byte_offset(); });
	std::transform(attribs.begin(), attribs.end(), std::back_inserter(buffers), [](const auto& buffer) { return buffer.vk_buffer(); });

	vk_cmd_buffer().bindVertexBuffers(u32(0), vk::ArrayProxy(attrib_count, buffers.cbegin()), vk::ArrayProxy(attrib_count, offsets.cbegin()));
}

const Viewport& RenderPassRecorder::viewport() const {
	return _viewport;
}

CmdBufferRegion RenderPassRecorder::region(const char* name, const math::Vec4& color) {
	return _cmd_buffer.region(name, color);
}

DevicePtr RenderPassRecorder::device() const {
	return _cmd_buffer.device();
}

vk::CommandBuffer RenderPassRecorder::vk_cmd_buffer() const {
	return _cmd_buffer.vk_cmd_buffer();
}


// -------------------------------------------------- CmdBufferRecorder --------------------------------------------------

CmdBufferRecorder::CmdBufferRecorder(CmdBufferBase&& base, CmdBufferUsage usage)  : CmdBufferBase(std::move(base)) {
	auto info = vk::CommandBufferBeginInfo()
			.setFlags(cmd_usage(usage))
		;

	vk_cmd_buffer().begin(info);
}

CmdBufferRecorder::~CmdBufferRecorder() {
	if(device()) {
		if(_render_pass) {
			y_fatal("CmdBufferRecorder destroyed before one of its RenderPassRecorder.");
		}
		if(vk_cmd_buffer()) {
			y_fatal("CmdBufferRecorder destroyed before end() was called.");
		}
	}
}

void CmdBufferRecorder::end_renderpass() {
	if(!_render_pass) {
		y_fatal("CmdBufferRecorder has no render pass");
	}
	vk_cmd_buffer().endRenderPass();
	_render_pass = nullptr;
}

void CmdBufferRecorder::check_no_renderpass() const {
	if(_render_pass) {
		y_fatal("This command can not be used while this command buffer has a RenderPassRecorder.");
	}
}

CmdBufferRegion CmdBufferRecorder::region(const char* name, const math::Vec4& color) {
	return CmdBufferRegion(*this, name, color);
}


RenderPassRecorder CmdBufferRecorder::bind_framebuffer(const Framebuffer& framebuffer) {
	check_no_renderpass();

	auto clear_values = core::vector_with_capacity<vk::ClearValue>(framebuffer.attachment_count() + 1);
	for(usize i = 0; i != framebuffer.attachment_count(); ++i) {
		clear_values << vk::ClearColorValue(std::array<float, 4>{{0.0f, 0.0f, 0.0f, 0.0f}});
	}
	clear_values << vk::ClearDepthStencilValue(0.0f, 0); // reversed Z

	auto pass_info = vk::RenderPassBeginInfo()
			.setRenderArea(vk::Rect2D({0, 0}, {framebuffer.size().x(), framebuffer.size().y()}))
			.setRenderPass(framebuffer.render_pass().vk_render_pass())
			.setFramebuffer(framebuffer.vk_framebuffer())
			.setPClearValues(clear_values.begin())
			.setClearValueCount(u32(clear_values.size()))
		;

	vk_cmd_buffer().beginRenderPass(pass_info, vk::SubpassContents::eInline);
	_render_pass = &framebuffer.render_pass();

	// set viewport
	auto size = framebuffer.size();
	vk_cmd_buffer().setViewport(0, {vk::Viewport(0, 0, size.x(), size.y(), 0.0f, 1.0f)});
	vk_cmd_buffer().setScissor(0, {vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(size.x(), size.y()))});

	return RenderPassRecorder(*this, Viewport(size));
}

void CmdBufferRecorder::dispatch(const ComputeProgram& program, const math::Vec3ui& size, DescriptorSetList descriptor_sets, const PushConstant& push_constants) {
	check_no_renderpass();

	auto ds = core::vector_with_capacity<vk::DescriptorSet>(descriptor_sets.size());
	std::transform(descriptor_sets.begin(), descriptor_sets.end(), std::back_inserter(ds), [](const auto& d) { return d.get().vk_descriptor_set(); });

	vk_cmd_buffer().bindPipeline(vk::PipelineBindPoint::eCompute, program.vk_pipeline());

	if(!ds.is_empty()) {
		vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eCompute, program.vk_pipeline_layout(), 0, ds.size(), ds.begin(), 0, nullptr);
	}

	if(!push_constants.is_empty()) {
		vk_cmd_buffer().pushConstants(program.vk_pipeline_layout(), vk::ShaderStageFlagBits::eCompute, 0, push_constants.size(), push_constants.data());
	}

	vk_cmd_buffer().dispatch(size.x(), size.y(), size.z());
}

void CmdBufferRecorder::dispatch_size(const ComputeProgram& program, const math::Vec3ui& size, DescriptorSetList descriptor_sets, const PushConstant& push_constants) {
	math::Vec3ui dispatch_size;
	for(usize i = 0; i != 3; ++i) {
		dispatch_size[i] = size[i] / program.local_size()[i] + !!(size[i] % program.local_size()[i]);
	}
	dispatch(program, dispatch_size, descriptor_sets, push_constants);
}

void CmdBufferRecorder::dispatch_size(const ComputeProgram& program, const math::Vec2ui& size, DescriptorSetList descriptor_sets, const PushConstant& push_constants) {
	dispatch_size(program, math::Vec3ui(size, 1), descriptor_sets, push_constants);
}

void CmdBufferRecorder::barriers(core::ArrayView<BufferBarrier> buffers, core::ArrayView<ImageBarrier> images) {
	check_no_renderpass();

	if(buffers.is_empty() && images.is_empty()) {
		return;
	}

	auto image_barriers = core::vector_with_capacity<vk::ImageMemoryBarrier>(images.size());
	std::transform(images.begin(), images.end(), std::back_inserter(image_barriers), [](const auto& b) { return b.vk_barrier(); });

	auto buffer_barriers = core::vector_with_capacity<vk::BufferMemoryBarrier>(buffers.size());
	std::transform(buffers.begin(), buffers.end(), std::back_inserter(buffer_barriers), [](const auto& b) { return b.vk_barrier(); });

	PipelineStage src_mask = PipelineStage::None;
	PipelineStage dst_mask = PipelineStage::None;

	for(const auto& b : buffers) {
		src_mask = src_mask | b.src_stage();
		dst_mask = dst_mask | b.dst_stage();
	}

	for(const auto& b : images) {
		src_mask = src_mask | b.src_stage();
		dst_mask = dst_mask | b.dst_stage();
	}

	vk_cmd_buffer().pipelineBarrier(
			vk::PipelineStageFlagBits(src_mask),
			vk::PipelineStageFlagBits(dst_mask),
			vk::DependencyFlagBits::eByRegion,
			0, nullptr,
			buffer_barriers.size(), buffer_barriers.begin(),
			image_barriers.size(), image_barriers.begin()
		);
}

void CmdBufferRecorder::barriers(core::ArrayView<BufferBarrier> buffers) {
	barriers(buffers, {});
}

void CmdBufferRecorder::barriers(core::ArrayView<ImageBarrier> images) {
	barriers({}, images);
}

void CmdBufferRecorder::copy(const SrcCopyBuffer& src, const DstCopyBuffer& dst) {
	if(src.byte_size() != dst.byte_size()) {
		y_fatal("Buffer size do not match.");
	}
	vk_cmd_buffer().copyBuffer(src.vk_buffer(), dst.vk_buffer(), vk::BufferCopy(src.byte_offset(), dst.byte_offset(), src.byte_size()));
}

void CmdBufferRecorder::copy(const SrcCopyImage& src, const DstCopyImage& dst) {
	if(src.size() != dst.size()) {
		y_fatal("Image size do not match.");
	}

	auto src_resource = vk::ImageSubresourceLayers()
		.setAspectMask(src.format().vk_aspect())
		.setMipLevel(0)
		.setBaseArrayLayer(0)
		.setLayerCount(1);
	auto dst_resource = vk::ImageSubresourceLayers()
		.setAspectMask(dst.format().vk_aspect())
		.setMipLevel(0)
		.setBaseArrayLayer(0)
		.setLayerCount(1);

	auto extent = vk::Extent3D(src.size().x(), src.size().y(), 1);

	vk_cmd_buffer().copyImage(src.vk_image(), vk_image_layout(src.usage()),
							  dst.vk_image(), vk_image_layout(dst.usage()), vk::ImageCopy(src_resource, vk::Offset3D(), dst_resource, vk::Offset3D(), extent));
}

void CmdBufferRecorder::blit(const SrcCopyImage& src, const DstCopyImage& dst) {
	vk::ImageBlit blit = vk::ImageBlit()
			.setSrcSubresource(
				vk::ImageSubresourceLayers()
					.setAspectMask(src.format().vk_aspect())
					.setLayerCount(1)
				)
			.setDstSubresource(
				 vk::ImageSubresourceLayers()
					 .setAspectMask(dst.format().vk_aspect())
					 .setLayerCount(1)
				)
		;

	vk_cmd_buffer().blitImage(src.vk_image(), vk_image_layout(src.usage()), dst.vk_image(), vk_image_layout(dst.usage()), blit, vk::Filter::eLinear);
}

void CmdBufferRecorder::transition_image(ImageBase& image, vk::ImageLayout src, vk::ImageLayout dst) {
	check_no_renderpass();

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

}
