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

#include "Swapchain.h"
#include "Device.h"
#include "Window.h"


namespace yave {


static vk::SurfaceCapabilitiesKHR compute_capabilities(DevicePtr dptr, vk::SurfaceKHR surface) {
	return dptr->get_physical_device().get_vk_physical_device().getSurfaceCapabilitiesKHR(surface);
}

static vk::SurfaceFormatKHR get_surface_format(DevicePtr dptr, vk::SurfaceKHR surface) {
	return dptr->get_physical_device().get_vk_physical_device().getSurfaceFormatsKHR(surface)[0];
}

static vk::PresentModeKHR get_present_mode(DevicePtr dptr, vk::SurfaceKHR surface) {
	auto present_modes = dptr->get_physical_device().get_vk_physical_device().getSurfacePresentModesKHR(surface);
	for(auto mode : present_modes) {
		if(mode == vk::PresentModeKHR::eMailbox) {
			return mode;
		}
	}
	return present_modes[0];
}

static u32 get_image_count(vk::SurfaceCapabilitiesKHR capabilities) {
	u32 ideal = 2;
	if(capabilities.maxImageCount < ideal) {
		return capabilities.maxImageCount;
	}
	if(capabilities.minImageCount > ideal) {
		return capabilities.minImageCount;
	}
	return ideal;
}

static void assert_depth_supported(DevicePtr dptr) {
	auto depth_props = dptr->get_physical_device().get_vk_physical_device().getFormatProperties(vk::Format::eD32Sfloat);

	if((depth_props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) != vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
		fatal("32 bit depth not supported");
	}
}

static vk::ImageView create_image_view(DevicePtr dptr, vk::Image image, vk::Format format) {
	auto mapping = vk::ComponentMapping()
			.setR(vk::ComponentSwizzle::eIdentity)
			.setG(vk::ComponentSwizzle::eIdentity)
			.setB(vk::ComponentSwizzle::eIdentity)
			.setA(vk::ComponentSwizzle::eIdentity)
		;

	auto subrange = vk::ImageSubresourceRange()
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseArrayLayer(0)
			.setBaseMipLevel(0)
			.setLayerCount(1)
			.setLevelCount(1)
		;

	return dptr->get_vk_device().createImageView(vk::ImageViewCreateInfo()
			.setImage(image)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(format)
			.setComponents(mapping)
			.setSubresourceRange(subrange)
		);
}

static bool has_wsi_support(DevicePtr dptr, vk::SurfaceKHR surface) {
	auto index = dptr->get_queue_family_index(QueueFamily::Graphics);
	return dptr->get_physical_device().get_vk_physical_device().getSurfaceSupportKHR(index, surface);
}

static vk::SurfaceKHR create_surface(DevicePtr dptr, Window* window) {
	#ifdef Y_OS_WIN
		auto surface = dptr->get_instance().get_vk_instance().createWin32SurfaceKHR(vk::Win32SurfaceCreateInfoKHR()
				.setHinstance(window->instance())
				.setHwnd(window->handle())
			);

		if(!has_wsi_support(dptr, surface)) {
			fatal("No WSI support");
		}
		log_msg("Vulkan WSI supported !");
		return surface;
	#endif
	return VK_NULL_HANDLE;
}




Swapchain::Buffer::Buffer(RenderPass& render_pass, SwapchainImage&& color_att, DepthAttachment&& depth_att) :
		color(std::move(color_att)),
		depth(std::move(depth_att)),
		framebuffer(render_pass, DepthAttachmentView(depth), ColorAttachmentView(color)) {
}

Swapchain::Swapchain(DevicePtr dptr, Window* window) : Swapchain(dptr, create_surface(dptr, window)) {
}

Swapchain::Swapchain(DevicePtr dptr, vk::SurfaceKHR&& surface) : DeviceLinked(dptr), _surface(surface) {
	auto capabilities = compute_capabilities(dptr, surface);
	auto format = get_surface_format(dptr, surface);

	_size = math::vec(capabilities.currentExtent.width, capabilities.currentExtent.height);
	_color_format = format.format;
	_depth_format = vk::Format::eD32Sfloat;

	assert_depth_supported(dptr);

	_render_pass = RenderPass(dptr, _depth_format, _color_format);

	_swapchain = dptr->get_vk_device().createSwapchainKHR(vk::SwapchainCreateInfoKHR()
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
			.setImageSharingMode(vk::SharingMode::eExclusive)
			.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
			.setImageArrayLayers(1)
			.setClipped(true)
			.setSurface(surface)
			.setPreTransform(capabilities.currentTransform)
			.setImageFormat(format.format)
			.setImageColorSpace(format.colorSpace)
			.setImageExtent(capabilities.currentExtent)
			.setMinImageCount(get_image_count(capabilities))
			.setPresentMode(get_present_mode(dptr, surface))
		);


	for(auto image : dptr->get_vk_device().getSwapchainImagesKHR(_swapchain)) {
		auto view = create_image_view(dptr, image, _color_format.get_vk_format());

		auto swapchain_image = SwapchainImage();
		swapchain_image._size = _size;
		swapchain_image._format = _color_format;

		// prevent the images to delete their handles: the swapchain already does that.
		swapchain_image._image = image;
		swapchain_image._memory = VK_NULL_HANDLE;
		swapchain_image._view = view;

		_buffers << Buffer(_render_pass, std::move(swapchain_image), DepthAttachment(dptr, _depth_format, _size));
	}
}

Swapchain::~Swapchain() {
	// images don't delete their views, we have to do it manually
	for(const Buffer& buffer : _buffers) {
		get_device()->destroy(buffer.color._view);
	}
	destroy(_swapchain);
	destroy(_surface);
}


vk::SwapchainKHR Swapchain::get_vk_swapchain() const {
	return _swapchain;
}

const Framebuffer& Swapchain::get_framebuffer(usize index) const {
	return _buffers[index].framebuffer;
}

const RenderPass& Swapchain::get_render_pass() const {
	return _render_pass;
}

const math::Vec2ui& Swapchain::size() const {
	return _size;
}

usize Swapchain::buffer_count() const {
	return _buffers.size();
}


}
