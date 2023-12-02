/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include <yave/window/Window.h>

#include <yave/graphics/commands/CmdQueue.h>
#include <yave/graphics/memory/DeviceMemoryHeapBase.h>
#include <yave/graphics/barriers/Barrier.h>

#include <yave/graphics/device/extensions/DebugUtils.h>

#include <y/core/FixedArray.h>
#include <y/core/ScratchPad.h>
#include <y/utils/log.h>

namespace yave {

static VkSurfaceCapabilitiesKHR compute_capabilities(VkSurfaceKHR surface) {
    VkSurfaceCapabilitiesKHR capabilities = {};
    if(is_error(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device(), surface, &capabilities))) {
        return {};
    }
    return capabilities;
}

static VkSurfaceFormatKHR surface_format(VkSurfaceKHR surface) {
    std::array<VkSurfaceFormatKHR, 16> formats = {};
    u32 format_count = u32(formats.size());
    vk_check_or_incomplete(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device(), surface, &format_count, formats.data()));
    y_always_assert(format_count, "No surface format supported");

    for(u32 i = 0; i != format_count; ++i) {
        switch(formats[i].format) {
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_B8G8R8A8_SRGB:
                return formats[i];

            default:
            break;
        }
    }

    return formats[0];
}

static VkPresentModeKHR present_mode(VkSurfaceKHR surface) {
    std::array<VkPresentModeKHR, 16> modes = {};
    u32 mode_count = u32(modes.size());
    vk_check(vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device(), surface, &mode_count, modes.data()));
    y_always_assert(mode_count, "No presentation mode supported");

    for(u32 i = 0; i != mode_count; ++i) {
        if(modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

static u32 compute_image_count(VkSurfaceCapabilitiesKHR capabilities) {
    const u32 ideal = 3;
    if(capabilities.maxImageCount < ideal && capabilities.maxImageCount > capabilities.minImageCount) {
        return capabilities.maxImageCount;
    }
    if(capabilities.minImageCount > ideal) {
        return capabilities.minImageCount;
    }
    return ideal;
}

static VkHandle<VkImageView> create_image_view(VkImage image, VkFormat format) {
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

    VkHandle<VkImageView> view;
    vk_check(vkCreateImageView(vk_device(), &create_info, vk_allocation_callbacks(), view.get_ptr_for_init()));
    return view;
}

static bool has_wsi_support(VkSurfaceKHR surface) {
    const u32 index =  command_queue().family_index();
    VkBool32 supported = false;
    vk_check(vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device(), index, surface, &supported));
    return supported;
}

#ifdef Y_OS_WIN
static VkHandle<VkSurfaceKHR> create_surface(HINSTANCE_ instance, HWND_ handle) {
    VkWin32SurfaceCreateInfoKHR create_info = vk_struct();
    {
        create_info.hinstance = instance;
        create_info.hwnd = handle;
    }

    VkHandle<VkSurfaceKHR> surface;
    vk_check(vkCreateWin32SurfaceKHR(vk_device_instance(), &create_info, vk_allocation_callbacks(), surface.get_ptr_for_init()));

    if(!has_wsi_support(surface)) {
        y_fatal("No WSI support.");
    }
    log_msg("Vulkan WSI supported!");

    return surface;
}
#endif

#ifdef Y_OS_LINUX
static VkSurfaceKHR create_surface(xcb_connection_t* connection, u32 window) {
    VkXcbSurfaceCreateInfoKHR create_info = vk_struct();
    {
        create_info.connection = connection;
        create_info.window = window;
    }

    VkSurfaceKHR surface = {};
    vk_check(vkCreateXcbSurfaceKHR(vk_device_instance(), &create_info, vk_allocation_callbacks(), &surface));

    if(!has_wsi_support(surface)) {
        y_fatal("No WSI support.");
    }
    log_msg("Vulkan WSI supported!");

    return surface;
}
#endif

static VkHandle<VkSurfaceKHR> create_surface(Window* window) {
    y_profile();

#if defined(Y_OS_WIN) || defined(Y_OS_LINUX)
    return create_surface(window->instance(), window->handle());
#endif

    unused(window);
    return {};
}


Swapchain::Swapchain(Window* window) : Swapchain(create_surface(window)) {
}

Swapchain::Swapchain(VkHandle<VkSurfaceKHR> surface) : _surface(std::move(surface)) {
    build_swapchain();
    build_sync_objects();
}

bool Swapchain::reset() {
    wait_all_queues();

    _images.clear();

    destroy_sync_objects();
    y_defer(build_sync_objects());

    VkHandle<VkSwapchainKHR> old;
    *old.get_ptr_for_init() = _swapchain.get();

    if(build_swapchain()) {
        destroy_graphic_resource(std::move(old));
        return true;
    } else {
        old.consume();
    }

    return false;
}

Swapchain::~Swapchain() {
    _images.clear();

    destroy_sync_objects();

    destroy_graphic_resource(std::move(_swapchain));
    destroy_graphic_resource(std::move(_surface));
}

bool Swapchain::build_swapchain() {
    y_debug_assert(_images.is_empty());

    const VkSurfaceCapabilitiesKHR capabilities = compute_capabilities(_surface);

    _size = {capabilities.currentExtent.width, capabilities.currentExtent.height};

    if(!_size.x() || !_size.y()) {
        return false;
    }

    const VkSurfaceFormatKHR format = surface_format(_surface);
    _color_format = VkFormat(format.format);

    const VkImageUsageFlagBits image_usage_flags = VkImageUsageFlagBits(SwapchainImageUsage & ~ImageUsage::SwapchainBit);
    if((capabilities.supportedUsageFlags & image_usage_flags) != image_usage_flags) {
        y_fatal("Swapchain does not support required usage flags.");
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
            create_info.presentMode = present_mode(_surface);
            create_info.oldSwapchain = _swapchain.consume();
        }
        vk_check(vkCreateSwapchainKHR(vk_device(), &create_info, vk_allocation_callbacks(), _swapchain.get_ptr_for_init()));
    }

    y_profile_zone("image setup");

    u32 image_count = 0;
    vk_check(vkGetSwapchainImagesKHR(vk_device(), _swapchain, &image_count, nullptr));

    core::FixedArray<VkImage> images(image_count);
    vk_check(vkGetSwapchainImagesKHR(vk_device(), _swapchain, &image_count, images.data()));

    for(const VkImage image : images) {
        auto view = create_image_view(image, _color_format.vk_format());

        if(const auto* debug = debug_utils()) {
            debug->set_resource_name(image, "Swapchain Image");
            debug->set_resource_name(view, "Swapchain Image View");
        }

        struct SwapchainImageMemory : DeviceMemory {
            SwapchainImageMemory() : DeviceMemory({}, 0, 0) {
            }
        };

        VkHandle<VkImage> image_handle;
        *image_handle.get_ptr_for_init() = image;

        auto swapchain_image = SwapchainImage();
        swapchain_image._memory = SwapchainImageMemory();
        swapchain_image._size = math::Vec3ui(_size, 1);
        swapchain_image._format = _color_format;
        swapchain_image._usage = SwapchainImageUsage;
        swapchain_image._image = std::move(image_handle);
        swapchain_image._view = std::move(view);

        _images << std::move(swapchain_image);
    }

    CmdBufferRecorder recorder = create_disposable_cmd_buffer();
    for(auto& i : _images) {
        recorder.barriers({ImageBarrier::transition_barrier(i, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)});
    }
    recorder.submit().wait();

    return true;
}

void Swapchain::build_sync_objects() {
    y_debug_assert(_sync_objects.is_empty());
    y_debug_assert(_image_fences.is_empty());

    const VkSemaphoreCreateInfo semaphore_create_info = vk_struct();
    VkFenceCreateInfo fence_create_info = vk_struct();
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(usize i = 0; i != image_count(); ++i) {
        auto& sync = _sync_objects.emplace_back();
        vk_check(vkCreateSemaphore(vk_device(), &semaphore_create_info, vk_allocation_callbacks(), sync.render_complete.get_ptr_for_init()));
        vk_check(vkCreateSemaphore(vk_device(), &semaphore_create_info, vk_allocation_callbacks(), sync.image_available.get_ptr_for_init()));
        vk_check(vkCreateFence(vk_device(), &fence_create_info, vk_allocation_callbacks(), sync.fence.get_ptr_for_init()));

        _image_fences.emplace_back();
    }

    y_debug_assert(image_count() == _sync_objects.size());
    y_debug_assert(image_count() == _image_fences.size());
}

void Swapchain::destroy_sync_objects() {
    for(auto& sync : _sync_objects) {
        vk_check(vkWaitForFences(vk_device(), 1, &sync.fence.get(), true, u64(-1)));

        destroy_graphic_resource(std::move(sync.render_complete));
        destroy_graphic_resource(std::move(sync.image_available));
        destroy_graphic_resource(std::move(sync.fence));
    }

    _sync_objects.clear();
    _image_fences.clear();
}

core::Result<FrameToken> Swapchain::next_frame() {
    y_profile();

    y_profile_frame_begin();

    if(_images.is_empty()) {
        if(!reset()) {
            return core::Err();
        }
    }

    y_debug_assert(_sync_objects.size());

    const usize current_frame_index = _frame_id % image_count();
    // we need to do the lookup every time, in case _sync_objects gets rebuild
    auto current_sync = [=]() -> FrameSyncObjects& { return _sync_objects[current_frame_index]; };

    vk_check(vkWaitForFences(vk_device(), 1, &current_sync().fence.get(), true, u64(-1)));

    u32 image_index = u32(-1);
    {
        y_profile_zone("aquire");
        while(vk_swapchain_out_of_date(vkAcquireNextImageKHR(vk_device(), _swapchain, u64(-1), current_sync().image_available, {}, &image_index))) {
            if(!reset()) {
                return core::Err();
            }
        }
    }

    y_debug_assert(image_index < _images.size());

    return core::Ok(FrameToken {
        _frame_id,
        image_index,
        u32(_images.size()),
        _swapchain.get(),
        SwapchainImageView(_images[image_index]),
    });
}

void Swapchain::present(const FrameToken& token, CmdBufferRecorder&& recorder, CmdQueue& queue) {
    y_profile();

    const usize frame_index = token.id % image_count();
    if(_image_fences[frame_index]) {
        vk_check(vkWaitForFences(vk_device(), 1, &_image_fences[frame_index], true, u64(-1)));
    }

    const usize current_frame_index = _frame_id % image_count();
    FrameSyncObjects& current_frame_sync = _sync_objects[current_frame_index];

    _image_fences[frame_index] = current_frame_sync.fence;
    vk_check(vkResetFences(vk_device(), 1, &current_frame_sync.fence.get()));

    if(vk_swapchain_out_of_date(queue.present(std::move(recorder), token, current_frame_sync))) {
        // Nothing ?
    }

    ++_frame_id;

    y_profile_frame_end();
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
