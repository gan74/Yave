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
#include "RenderPass.h"

#include <yave/image/Image.h>

#include "Device.h"

namespace yave {

static vk::ImageLayout vk_attachment_image_layout(ImageUsage usage) {
	return vk_image_layout(usage & ImageUsage::Attachment);
}

static vk::SubpassDescription create_subpass(const vk::AttachmentReference& depth, const core::Vector<vk::AttachmentReference>& colors) {
	return vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(u32(colors.size()))
		.setPColorAttachments(colors.begin())
		.setPDepthStencilAttachment(&depth)
	;
}

static std::array<vk::SubpassDependency, 1> create_bottom_of_pipe_dependencies() {
	return {{vk::SubpassDependency()
		.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
		.setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
		.setDstAccessMask(
			vk::AccessFlagBits::eColorAttachmentRead |
			vk::AccessFlagBits::eColorAttachmentWrite |
			vk::AccessFlagBits::eDepthStencilAttachmentRead |
			vk::AccessFlagBits::eDepthStencilAttachmentWrite
		)/*,
		vk::SubpassDependency()
			.setSrcSubpass(0)
			.setDstSubpass(VK_SUBPASS_EXTERNAL)
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setSrcAccessMask(
					vk::AccessFlagBits::eColorAttachmentRead |
					vk::AccessFlagBits::eColorAttachmentWrite |
					vk::AccessFlagBits::eDepthStencilAttachmentRead |
					vk::AccessFlagBits::eDepthStencilAttachmentWrite
			)
			.setDstStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
			.setDstAccessMask(vk::AccessFlagBits::eMemoryRead)*/
		}};
}

static vk::AttachmentDescription create_attachment(RenderPass::ImageData image) {
	return vk::AttachmentDescription()
		.setFormat(image.format.vk_format())
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setFinalLayout(vk_image_layout(image.usage))
	;
}

static vk::AttachmentReference create_attachment_reference(ImageUsage usage, usize index) {
	return vk::AttachmentReference()
		.setAttachment(u32(index))
		.setLayout(vk_attachment_image_layout(usage))
	;
}


static vk::RenderPass create_renderpass(DevicePtr dptr, RenderPass::ImageData depth, const core::Vector<RenderPass::ImageData>& colors) {
	auto attachments =
			core::range(colors).map([=](const auto& color) { return create_attachment(color); }).collect<core::Vector>() +
			create_attachment(depth);

	auto color_refs = core::range(usize(0), colors.size()).map([](auto i) { return create_attachment_reference(ImageUsage::ColorBit, i); }).collect<core::Vector>();
	auto depth_ref = create_attachment_reference(ImageUsage::DepthBit, color_refs.size());

	auto subpass = create_subpass(depth_ref, color_refs);

	auto dependencies = create_bottom_of_pipe_dependencies();

	return dptr->vk_device().createRenderPass(vk::RenderPassCreateInfo()
			.setAttachmentCount(u32(attachments.size()))
			.setPAttachments(attachments.begin())
			.setSubpassCount(1)
			.setPSubpasses(&subpass)
			.setDependencyCount(dependencies.size())
			.setPDependencies(dependencies.begin())
		)
	;
}




RenderPass::RenderPass(DevicePtr dptr, ImageData depth, const core::Vector<ImageData>& colors) :
		DeviceLinked(dptr),
		_attachment_count(colors.size()),
		_render_pass(create_renderpass(dptr, depth, colors)) {
}

RenderPass::~RenderPass() {
	destroy(_render_pass);
}

RenderPass::RenderPass(RenderPass&& other) {
	swap(other);
}

RenderPass& RenderPass::operator=(RenderPass&& other) {
	swap(other);
	return *this;
}

void RenderPass::swap(RenderPass& other) {
	DeviceLinked::swap(other);
	std::swap(_attachment_count, other._attachment_count);
	std::swap(_render_pass, other._render_pass);
}

vk::RenderPass RenderPass::vk_render_pass() const {
	return _render_pass;
}


}
