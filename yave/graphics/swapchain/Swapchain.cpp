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

#include "Swapchain.h"

#include <yave/device/Device.h>
#include <yave/window/Window.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/memory/DeviceMemoryHeapBase.h>


namespace yave {


static vk::SurfaceCapabilitiesKHR compute_capabilities(DevicePtr dptr, vk::SurfaceKHR surface) {
	y_profile();
	return dptr->physical_device().vk_physical_device().getSurfaceCapabilitiesKHR(surface);
}

static vk::SurfaceFormatKHR get_surface_format(DevicePtr dptr, vk::SurfaceKHR surface) {
	y_profile();
	return dptr->physical_device().vk_physical_device().getSurfaceFormatsKHR(surface).front();
}

static vk::PresentModeKHR get_present_mode(DevicePtr dptr, vk::SurfaceKHR surface) {
	auto present_modes = dptr->physical_device().vk_physical_device().getSurfacePresentModesKHR(surface);
	for(auto mode : present_modes) {
		if(mode == vk::PresentModeKHR::eMailbox) {
			return mode;
		}
	}
	return present_modes[0];
}

static u32 get_image_count(vk::SurfaceCapabilitiesKHR capabilities) {
	u32 ideal = 3;
	if(capabilities.maxImageCount < ideal) {
		return capabilities.maxImageCount;
	}
	if(capabilities.minImageCount > ideal) {
		return capabilities.minImageCount;
	}
	return ideal;
}

/*static void assert_depth_supported(DevicePtr dptr) {
	auto depth_props = dptr->physical_device().vk_physical_device().getFormatProperties(vk::Format::eD32Sfloat);

	if((depth_props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) != vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
		y_fatal("32 bit depth not supported.");
	}
}*/

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

	return dptr->vk_device().createImageView(vk::ImageViewCreateInfo()
			.setImage(image)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(format)
			.setComponents(mapping)
			.setSubresourceRange(subrange)
		);
}

static bool has_wsi_support(DevicePtr dptr, vk::SurfaceKHR surface) {
	auto index = dptr->queue_family(QueueFamily::Graphics).index();
	return dptr->physical_device().vk_physical_device().getSurfaceSupportKHR(index, surface);
}

#ifdef Y_OS_WIN
static vk::SurfaceKHR create_surface(DevicePtr dptr, HINSTANCE instance, HWND handle) {
	auto surface = dptr->instance().vk_instance().createWin32SurfaceKHR(vk::Win32SurfaceCreateInfoKHR()
			.setHinstance(instance)
			.setHwnd(handle)
		);

	if(!has_wsi_support(dptr, surface)) {
		y_fatal("No WSI support.");
	}
	log_msg("Vulkan WSI supported!");
	return surface;
}
#endif

static vk::SurfaceKHR create_surface(DevicePtr dptr, Window* window) {
	y_profile();
	#ifdef Y_OS_WIN
		return create_surface(dptr, window->instance(), window->handle());
	#endif
	return vk::SurfaceKHR();
}





#ifdef Y_OS_WIN
Swapchain::Swapchain(DevicePtr dptr, HINSTANCE instance, HWND handle) : Swapchain(dptr, create_surface(dptr, instance, handle)) {
}
#endif

Swapchain::Swapchain(DevicePtr dptr, Window* window) : Swapchain(dptr, create_surface(dptr, window)) {
}

Swapchain::Swapchain(DevicePtr dptr, vk::SurfaceKHR&& surface) : DeviceLinked(dptr), _surface(surface) {
	build_swapchain();
	build_semaphores();
	y_debug_assert(_images.size() == _semaphores.size());
}

bool Swapchain::is_valid() const {
	return _size.x() > 0 && _size.y() > 0 && !_images.is_empty();
}

void Swapchain::reset() {
	_images.clear();
	destroy_semaphores();

	auto old = _swapchain;

	build_swapchain();
	build_semaphores();

	destroy(old);

	y_debug_assert(_images.size() == _semaphores.size());
}

Swapchain::~Swapchain() {
	_images.clear();

	destroy_semaphores();

	destroy(_swapchain);
	destroy(_surface);
}

void Swapchain::build_swapchain() {
	auto capabilities = compute_capabilities(device(), _surface);
	auto format = get_surface_format(device(), _surface);

	auto image_usage_flags = vk::ImageUsageFlagBits(SwapchainImageUsage & ~ImageUsage::SwapchainBit);
	if((capabilities.supportedUsageFlags & image_usage_flags) != image_usage_flags) {
		y_fatal("Swapchain does not support required usage flags.");
	}

	_size = {capabilities.currentExtent.width, capabilities.currentExtent.height};
	_color_format = format.format;

	if(!_size.x() || !_size.y()) {
		return;
	}

	{
		y_profile_zone("create swapchain");
		_swapchain = device()->vk_device().createSwapchainKHR(vk::SwapchainCreateInfoKHR()
				.setImageUsage(image_usage_flags)
				.setImageSharingMode(vk::SharingMode::eExclusive)
				.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
				.setImageArrayLayers(1)
				.setClipped(true)
				.setSurface(_surface)
				.setPreTransform(capabilities.currentTransform)
				.setImageFormat(format.format)
				.setImageColorSpace(format.colorSpace)
				.setImageExtent(capabilities.currentExtent)
				.setMinImageCount(get_image_count(capabilities))
				.setPresentMode(get_present_mode(device(), _surface))
				.setOldSwapchain(_swapchain)
			);
	}

	y_profile_zone("image setup");
	for(auto image : device()->vk_device().getSwapchainImagesKHR(_swapchain)) {
		auto view = create_image_view(device(), image, _color_format.vk_format());

		struct SwapchainImageMemory : DeviceMemory {
			SwapchainImageMemory(DevicePtr dptr) : DeviceMemory(dptr, vk::DeviceMemory(), 0, 0) {
			}
		};

		auto swapchain_image = SwapchainImage();

		swapchain_image._memory = SwapchainImageMemory(device());

		swapchain_image._size = math::Vec3ui(_size, 1);
		swapchain_image._format = _color_format;
		swapchain_image._usage = SwapchainImageUsage;

		// prevent the images to delete their handles: the swapchain already does that.
		swapchain_image._image = image;
		swapchain_image._view = view;

		_images << std::move(swapchain_image);
	}

	CmdBufferRecorder recorder(device()->create_disposable_cmd_buffer());
	for(auto& i : _images) {
		recorder.barriers({ImageBarrier::transition_barrier(i, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR)});
	}
	device()->graphic_queue().submit<SyncSubmit>(RecordedCmdBuffer(std::move(recorder)));
}

void Swapchain::build_semaphores() {
	for(usize i = 0; i != _images.size(); ++i) {
		_semaphores << std::pair(device()->vk_device().createSemaphore(vk::SemaphoreCreateInfo()),
								 device()->vk_device().createSemaphore(vk::SemaphoreCreateInfo()));
	}
}

void Swapchain::destroy_semaphores() {
	for(const auto& semaphores : _semaphores) {
		destroy(semaphores.first);
		destroy(semaphores.second);
	}
	_semaphores.clear();
}

FrameToken Swapchain::next_frame() {
	y_debug_assert(_semaphores.size());
	y_debug_assert(is_valid());

	auto [image_acquired, render_finished] = _semaphores.pop();
	u32 image_index = device()->vk_device().acquireNextImageKHR(_swapchain, u64(-1), image_acquired, vk::Fence()).value;

	return FrameToken {
			++_frame_id,
			image_index,
			u32(image_count()),
			SwapchainImageView(_images[image_index]),
			image_acquired,
			render_finished,
		};
}

void Swapchain::present(const FrameToken& token, vk::Queue queue) {
	_semaphores << std::pair(token.image_aquired, token.render_finished);
	std::rotate(_semaphores.begin(), _semaphores.end() - 1, _semaphores.end());

	queue.presentKHR(vk::PresentInfoKHR()
			.setSwapchainCount(1)
			.setPSwapchains(&_swapchain)
			.setPImageIndices(&token.image_index)
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&token.render_finished)
		);
}

vk::SwapchainKHR Swapchain::vk_swapchain() const {
	return _swapchain;
}

const math::Vec2ui& Swapchain::size() const {
	return _size;
}

usize Swapchain::image_count() const {
	return _images.size();
}

ImageFormat Swapchain::color_format() const {
	return _color_format;
}


}
