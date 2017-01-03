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

static auto create_barrier(
		vk::Image image,
		ImageFormat format,
		vk::ImageLayout old_layout,
		vk::AccessFlags src_access,
		vk::ImageLayout new_layout,
		vk::AccessFlags dst_access) {

	return vk::ImageMemoryBarrier()
			.setOldLayout(old_layout)
			.setNewLayout(new_layout)
			.setSrcAccessMask(src_access)
			.setDstAccessMask(dst_access)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setImage(image)
			.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(format.vk_aspect())
					.setBaseArrayLayer(0)
					.setBaseMipLevel(0)
					.setLayerCount(1)
					.setLevelCount(1)
				)
		;
}

static vk::AccessFlags vk_access_flags(vk::ImageLayout layout) {
	switch(layout) {
		case vk::ImageLayout::eUndefined:
			return vk::AccessFlags();

		case vk::ImageLayout::eColorAttachmentOptimal:
			return vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

		case vk::ImageLayout::eDepthStencilAttachmentOptimal:
			return vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
			return vk::AccessFlagBits::eDepthStencilAttachmentRead;

		case vk::ImageLayout::eShaderReadOnlyOptimal:
			return vk::AccessFlagBits::eShaderRead;

		case vk::ImageLayout::eTransferDstOptimal:
			return vk::AccessFlagBits::eTransferWrite;

		case vk::ImageLayout::ePresentSrcKHR:
			return vk::AccessFlags();

		default:
			break;
	}

	return fatal("Unsupported layout transition.");
}


CmdBufferRecorder::CmdBufferRecorder(CmdBuffer&& buffer) : _cmd_buffer(std::move(buffer)), _render_pass(nullptr) {
	Y_TODO(optimise disposable buffers)
	vk_cmd_buffer().begin(vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
		);
}

CmdBufferRecorder::CmdBufferRecorder(CmdBufferRecorder&& other) {
	swap(other);
}

CmdBufferRecorder& CmdBufferRecorder::operator=(CmdBufferRecorder&& other) {
	swap(other);
	return *this;
}

void CmdBufferRecorder::swap(CmdBufferRecorder& other) {
	std::swap(_cmd_buffer, other._cmd_buffer);
	std::swap(_render_pass, other._render_pass);

}

vk::CommandBuffer CmdBufferRecorder::vk_cmd_buffer() const {
	return _cmd_buffer.vk_cmd_buffer();
}


const RenderPass& CmdBufferRecorder::current_pass() const {
	return *_render_pass;
}

const Viewport& CmdBufferRecorder::viewport() const {
	return _viewport;
}

RecordedCmdBuffer CmdBufferRecorder::end() {
	end_render_pass();
	vk_cmd_buffer().end();
	return RecordedCmdBuffer(std::move(_cmd_buffer));
}

CmdBufferRecorder& CmdBufferRecorder::end_render_pass() {
	if(_render_pass) {
		vk_cmd_buffer().endRenderPass();
		_render_pass = nullptr;
	}
	return *this;
}

CmdBufferRecorder::~CmdBufferRecorder() {
	if(_cmd_buffer.vk_cmd_buffer()) {
		fatal("CmdBufferRecorder destroyed before end() was called.");
	}
}

CmdBufferRecorder& CmdBufferRecorder::set_viewport(const Viewport& view) {
	_viewport = view;
	return *this;
}

CmdBufferRecorder& CmdBufferRecorder::bind_framebuffer(const RenderPass& render_pass, const Framebuffer& framebuffer) {
	if(render_pass.vk_render_pass() != framebuffer.vk_render_pass()) {
		fatal("Incompatible render passes.");
	}

	if(_render_pass) {
		end_render_pass();
	}
	auto clear_values =
			core::range(usize(0), framebuffer.attachment_count()).map([](usize) { return vk::ClearValue(vk::ClearColorValue(std::array<float, 4>{{0.0f, 0.0f, 0.0f, 0.0f}})); }).collect<core::Vector>() +
			vk::ClearDepthStencilValue(1.0f, 0);

	auto pass_info = vk::RenderPassBeginInfo()
			.setRenderArea(vk::Rect2D({0, 0}, {framebuffer.size().x(), framebuffer.size().y()}))
			.setRenderPass(render_pass.vk_render_pass())
			.setFramebuffer(framebuffer.vk_framebuffer())
			.setPClearValues(clear_values.begin())
			.setClearValueCount(u32(clear_values.size()))
		;
	vk_cmd_buffer().beginRenderPass(pass_info, vk::SubpassContents::eInline);
	_render_pass = &render_pass;

	return set_viewport(Viewport(framebuffer.size()));
}

CmdBufferRecorder& CmdBufferRecorder::bind_pipeline(const GraphicPipeline& pipeline, const DescriptorSet& vp) {
	vk_cmd_buffer().bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.vk_pipeline());

	Y_TODO(descriptor set binding infecient)
	if(vp.vk_descriptor_set()) {
		auto vk = vp.vk_descriptor_set();
		vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.vk_pipeline_layout(), 0, 1, &vk, 0, nullptr);
	}
	if(pipeline.vk_descriptor_set()) {
		auto vk = pipeline.vk_descriptor_set();
		vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.vk_pipeline_layout(), 1, 1, &vk, 0, nullptr);
	}

	return *this;
}

CmdBufferRecorder& CmdBufferRecorder::draw(const StaticMeshInstance& mesh_instance) {
	vk::DeviceSize offset = 0; // fohkin' vk::ArrayProxy

	vk_cmd_buffer().bindVertexBuffers(0, mesh_instance.vertex_buffer.vk_buffer(), offset);
	vk_cmd_buffer().bindIndexBuffer(mesh_instance.triangle_buffer.vk_buffer(), offset, vk::IndexType::eUint32);

	auto cmds = u32(mesh_instance.indirect_buffer.size());
	vk_cmd_buffer().drawIndexedIndirect(mesh_instance.indirect_buffer.vk_buffer(), offset, cmds, cmds ? sizeof(vk::DrawIndexedIndirectCommand) : 0);

	return *this;
}


CmdBufferRecorder& CmdBufferRecorder::dispatch(const ComputeProgram& program, const math::Vec3ui& size, const DescriptorSet& descriptor_set) {
	end_render_pass();

	auto ds = descriptor_set.vk_descriptor_set();

	vk_cmd_buffer().bindPipeline(vk::PipelineBindPoint::eCompute, program.vk_pipeline());
	vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eCompute, program.vk_pipeline_layout(), 0, 1, &ds, 0, nullptr);
	vk_cmd_buffer().dispatch(size.x(), size.y(), size.z());

	return *this;
}

CmdBufferRecorder& CmdBufferRecorder::image_barriers(std::initializer_list<std::reference_wrapper<ImageBase>> images, PipelineStage src, PipelineStage dst) {
	if(_render_pass) {
		fatal("Image barrier inside renderpass.");
	}

	auto barriers = core::vector_with_capacity<vk::ImageMemoryBarrier>(images.size());
	for(ImageBase& image : images) {
		auto shader_usage = vk_image_layout(image.usage());
		auto shader_access_flags = vk_access_flags(shader_usage);

		barriers << create_barrier(
				image.vk_image(),
				image.format(),
				shader_usage, shader_access_flags,
				shader_usage, shader_access_flags
			);
	}

	vk_cmd_buffer().pipelineBarrier(
			vk::PipelineStageFlagBits(src),
			vk::PipelineStageFlagBits(dst),
			vk::DependencyFlagBits::eByRegion,
			0, nullptr,
			0, nullptr,
			barriers.size(), barriers.begin()
		);

	return *this;
}

CmdBufferRecorder& CmdBufferRecorder::transition_image(ImageBase& image, vk::ImageLayout src, vk::ImageLayout dst) {
	//log_msg("Image "_s + image.vk_image() + " transitioned from " + uenum(src) + " to " + uenum(dst));

	auto barrier = create_barrier(
			image.vk_image(),
			image.format(),
			src, vk_access_flags(src),
			dst, vk_access_flags(dst)
		);

	vk_cmd_buffer().pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::DependencyFlagBits::eByRegion,
			nullptr, nullptr, barrier
		);
	return *this;
}


}
