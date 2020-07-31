/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include <y/utils/log.h>

namespace yave {

static VkSurfaceCapabilitiesKHR compute_capabilities(DevicePtr dptr, VkSurfaceKHR surface) {
    y_profile();
    VkSurfaceCapabilitiesKHR capabilities = {};
    vk_check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dptr->vk_physical_device(), surface, &capabilities));
    return capabilities;
}

static VkSurfaceFormatKHR surface_format(DevicePtr dptr, VkSurfaceKHR surface) {
    y_profile();

    Y_TODO(Find best format instead of always returning first)
    u32 format_count = 1;
    VkSurfaceFormatKHR format = {};
    vk_check_or_incomplete(vkGetPhysicalDeviceSurfaceFormatsKHR(dptr->vk_physical_device(), surface, &format_count, &format));
    y_always_assert(format_count, "No swapchain format supported");
    return format;
}

static VkPresentModeKHR present_mode(DevicePtr dptr, VkSurfaceKHR surface) {
    std::array<VkPresentModeKHR, 16> modes = {};
    u32 mode_count = modes.size();
    vk_check(vkGetPhysicalDeviceSurfacePresentModesKHR(dptr->vk_physical_device(), surface, &mode_count, modes.data()));
    y_always_assert(mode_count, "No presentation mode supported");
    for(u32 i = 0; i != mode_count; ++i) {
        if(modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return modes[i];
        }
    }
    return modes[0];
}

static u32 compute_image_count(VkSurfaceCapabilitiesKHR capabilities) {
    const u32 ideal = 3;
    if(capabilities.maxImageCount < ideal) {
        return capabilities.maxImageCount;
    }
    if(capabilities.minImageCount > ideal) {
        return capabilities.minImageCount;
    }
    return ideal;
}

static VkImageView create_image_view(DevicePtr dptr, VkImage image, VkFormat format) {
    const VkComponentMapping mapping = {
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY
    };

    VkImageSubresourceRange subrange = {};
    {
        subrange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subrange.layerCount = 1;
        subrange.levelCount = 1;
    }

    VkImageViewCreateInfo create_info = vk_struct();
    {
        create_info.image = image;
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = format;
        create_info.components = mapping;
        create_info.subresourceRange = subrange;
    }

    VkImageView view = {};
    vk_check(vkCreateImageView(vk_device(dptr), &create_info, vk_allocation_callbacks(dptr), &view));
    return view;
}

static bool has_wsi_support(DevicePtr dptr, VkSurfaceKHR surface) {
    const u32 index = dptr->queue_family(QueueFamily::Graphics).index();
    VkBool32 supported = false;
    vk_check(vkGetPhysicalDeviceSurfaceSupportKHR(dptr->vk_physical_device(), index, surface, &supported));
    return supported;
}

#ifdef Y_OS_WIN
static VkSurfaceKHR create_surface(DevicePtr dptr, HINSTANCE_ instance, HWND_ handle) {
    VkWin32SurfaceCreateInfoKHR create_info = vk_struct();
    {
        create_info.hinstance = instance;
        create_info.hwnd = handle;
    }

    VkSurfaceKHR surface = {};
    vk_check(vkCreateWin32SurfaceKHR(dptr->instance().vk_instance(), &create_info, vk_allocation_callbacks(dptr), &surface));

    if(!has_wsi_support(dptr, surface)) {
        y_fatal("No WSI support.");
    }
    log_msg("Vulkan WSI supported!");

    return surface;
}
#endif

static VkSurfaceKHR create_surface(DevicePtr dptr, Window* window) {
    y_profile();
    #ifdef Y_OS_WIN
        return create_surface(dptr, window->instance(), window->handle());
    #endif
    return vk_null();
}





#ifdef Y_OS_WIN
Swapchain::Swapchain(DevicePtr dptr, HINSTANCE_ instance, HWND_ handle) : Swapchain(dptr, create_surface(dptr, instance, handle)) {
}
#endif

Swapchain::Swapchain(DevicePtr dptr, Window* window) : Swapchain(dptr, create_surface(dptr, window)) {
}

Swapchain::Swapchain(DevicePtr dptr, VkSurfaceKHR surface) : DeviceLinked(dptr), _surface(surface) {
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

    const auto old = _swapchain;

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
    const VkSurfaceCapabilitiesKHR capabilities = compute_capabilities(device(), _surface);
    const VkSurfaceFormatKHR format = surface_format(device(), _surface);

    const VkImageUsageFlagBits image_usage_flags = VkImageUsageFlagBits(SwapchainImageUsage & ~ImageUsage::SwapchainBit);
    if((capabilities.supportedUsageFlags & image_usage_flags) != image_usage_flags) {
        y_fatal("Swapchain does not support required usage flags.");
    }

    _size = {capabilities.currentExtent.width, capabilities.currentExtent.height};
    _color_format = VkFormat(format.format);

    if(!_size.x() || !_size.y()) {
        return;
    }

    {
        y_profile_zone("create swapchain");
        VkSwapchainCreateInfoKHR create_info = vk_struct();
        {
            create_info.imageUsage = image_usage_flags;
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            create_info.imageArrayLayers = 1;
            create_info.clipped = true;
            create_info.surface = _surface;
            create_info.preTransform = capabilities.currentTransform;
            create_info.imageFormat = format.format;
            create_info.imageColorSpace = format.colorSpace;
            create_info.imageExtent = capabilities.currentExtent;
            create_info.minImageCount = compute_image_count(capabilities);
            create_info.presentMode = present_mode(device(), _surface);
            create_info.oldSwapchain = _swapchain;
        }
        vk_check(vkCreateSwapchainKHR(device()->vk_device(), &create_info, device()->vk_allocation_callbacks(), &_swapchain));
    }

    y_profile_zone("image setup");

    core::Vector<VkImage> images;
    {
        u32 count = 0;
        vk_check(vkGetSwapchainImagesKHR(device()->vk_device(), _swapchain, &count, nullptr));
        images = core::Vector<VkImage>(count, VkImage{});
        vk_check(vkGetSwapchainImagesKHR(device()->vk_device(), _swapchain, &count, images.data()));
    }

    for(auto image : images) {
        const VkImageView view = create_image_view(device(), image, _color_format.vk_format());

        struct SwapchainImageMemory : DeviceMemory {
            SwapchainImageMemory(DevicePtr dptr) : DeviceMemory(dptr, vk_null(), 0, 0) {
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
        recorder.barriers({ImageBarrier::transition_barrier(i, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)});
    }
    device()->graphic_queue().submit<SyncSubmit>(RecordedCmdBuffer(std::move(recorder)));
}

void Swapchain::build_semaphores() {
    for(usize i = 0; i != _images.size(); ++i) {
        auto& semaphores = _semaphores.emplace_back();
        const VkSemaphoreCreateInfo create_info = vk_struct();
        vk_check(vkCreateSemaphore(device()->vk_device(), &create_info, device()->vk_allocation_callbacks(), &semaphores.first));
        vk_check(vkCreateSemaphore(device()->vk_device(), &create_info, device()->vk_allocation_callbacks(), &semaphores.second));
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

    u32 image_index = 0;
    vk_check(vkAcquireNextImageKHR(device()->vk_device(), _swapchain, u64(-1), image_acquired, vk_null(), &image_index));

    return FrameToken {
            ++_frame_id,
            image_index,
            u32(image_count()),
            SwapchainImageView(_images[image_index]),
            image_acquired,
            render_finished,
        };
}

void Swapchain::present(const FrameToken& token, VkQueue queue) {
    y_profile();
    _semaphores << std::pair(token.image_aquired, token.render_finished);
    std::rotate(_semaphores.begin(), _semaphores.end() - 1, _semaphores.end());

    {
        y_profile_zone("queue present");

        VkPresentInfoKHR present_info = vk_struct();
        {
            present_info.swapchainCount = 1;
            present_info.pSwapchains = reinterpret_cast<VkSwapchainKHR*>(&_swapchain);
            present_info.pImageIndices = &token.image_index;
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores = &token.render_finished;
        }

        vk_check(vkQueuePresentKHR(queue, &present_info));
    }
}

VkSwapchainKHR Swapchain::vk_swapchain() const {
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

