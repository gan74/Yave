/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/
#include "RenderPass.h"

#include "LowLevelGraphics.h"

namespace yave {

vk::AttachmentDescription create_attachment(ImageFormat format, ImageUsage usage) {
	return vk::AttachmentDescription()
		.setFormat(format.get_vk_format())
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setFinalLayout(usage.get_vk_image_layout())
	;
}

vk::AttachmentReference create_attachment_reference(ImageUsage usage, u32 index) {
	return vk::AttachmentReference()
		.setAttachment(index)
		.setLayout(usage.get_vk_image_layout())
	;
}

vk::SubpassDescription create_subpass(const vk::AttachmentReference& depth, const vk::AttachmentReference& colors) {
	return vk::SubpassDescription()
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setColorAttachmentCount(1)
		.setPColorAttachments(&colors)
		.setPDepthStencilAttachment(&depth)
	;
}

vk::SubpassDependency create_bottom_of_pipe_dependency() {
	return vk::SubpassDependency()
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
		)
	;
}

vk::RenderPass create_renderpass(DevicePtr dptr, ImageFormat depth_format, ImageFormat color_format) {
	auto attachments = {create_attachment(color_format, ImageUsageBits::SwapchainBit), create_attachment(depth_format, ImageUsageBits::DepthBit)};

	auto color_ref = create_attachment_reference(ImageUsageBits::ColorBit, 0);
	auto depth_ref = create_attachment_reference(ImageUsageBits::DepthBit, 1);

	auto subpass = create_subpass(depth_ref, color_ref);

	auto dependency = create_bottom_of_pipe_dependency();

	return dptr->get_vk_device().createRenderPass(vk::RenderPassCreateInfo()
		.setAttachmentCount(attachments.size())
		.setPAttachments(attachments.begin())
		.setSubpassCount(1)
		.setPSubpasses(&subpass)
		.setDependencyCount(1)
		.setPDependencies(&dependency)
	);
}


RenderPass::RenderPass(DevicePtr dptr, ImageFormat depth_format, ImageFormat color_format) : DeviceLinked(dptr), _render_pass(create_renderpass(dptr, depth_format, color_format)) {
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
	std::swap(_render_pass, other._render_pass);
}

vk::RenderPass RenderPass::get_vk_render_pass() const {
	return _render_pass;
}


}
