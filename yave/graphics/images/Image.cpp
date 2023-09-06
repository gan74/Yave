/*******************************
Copyright () 2016-2023 Grégoire Angerand

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

#include "Image.h"
#include "ImageData.h"

#include <yave/graphics/buffers/Buffer.h>
#include <yave/graphics/barriers/Barrier.h>
#include <yave/graphics/commands/CmdQueue.h>
#include <yave/graphics/memory/DeviceMemoryAllocator.h>
#include <yave/graphics/graphics.h>

#include <y/core/ScratchPad.h>

namespace yave {

static void bind_image_memory(VkImage image, const DeviceMemory& memory) {
    vk_check(vkBindImageMemory(vk_device(), image, memory.vk_memory(), memory.vk_offset()));
}

static VkHandle<VkImage> create_image(const math::Vec3ui& size, usize layers, usize mips, ImageFormat format, ImageUsage usage, ImageType type) {
    y_debug_assert(usage != ImageUsage::TransferDstBit);

    VkImageCreateInfo create_info = vk_struct();
    {
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.flags = type == ImageType::Cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
        create_info.arrayLayers = u32(layers);
        create_info.extent = {size.x(), size.y(), size.z()};
        create_info.format = format.vk_format();
        create_info.imageType = type == ImageType::ThreeD ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
        create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        create_info.mipLevels = u32(mips);
        create_info.usage = VkImageUsageFlags(usage);
        create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    }

    VkHandle<VkImage> image;
    vk_check(vkCreateImage(vk_device(), &create_info, vk_allocation_callbacks(), image.get_ptr_for_init()));
    return image;
}

static core::ScratchPad<VkBufferImageCopy> get_copy_regions(const ImageData& data) {
    core::ScratchPad<VkBufferImageCopy> regions(data.mipmaps());

    usize index = 0;
    for(usize m = 0; m != data.mipmaps(); ++m) {
        const auto size = data.mip_size(m);
        VkBufferImageCopy copy = {};
        {
            copy.bufferOffset = data.data_offset(m);
            copy.imageExtent = {size.x(), size.y(), size.z()};
            copy.imageSubresource.aspectMask = data.format().vk_aspect();
            copy.imageSubresource.mipLevel = u32(m);
            copy.imageSubresource.baseArrayLayer = 0;
            copy.imageSubresource.layerCount = 1;
        }
        regions[index++] = copy;
    }

    return regions;
}

static auto create_stagin_buffer(usize byte_size, const void* data) {
    y_profile();
    auto staging_buffer = StagingBuffer(byte_size);
    {
        y_profile_zone("copy");
        if(data) {
            std::memcpy(staging_buffer.map_bytes(MappingAccess::WriteOnly).data(), data, byte_size);
        } else {
            std::memset(staging_buffer.map_bytes(MappingAccess::WriteOnly).data(), 0, byte_size);
        }
    }
    return staging_buffer;
}

static VkHandle<VkImageView> create_view(VkImage image, ImageFormat format, u32 layers, u32 mips, ImageType type) {
    VkImageViewCreateInfo create_info = vk_struct();
    {
        create_info.image = image;
        create_info.viewType = VkImageViewType(type);
        create_info.format = format.vk_format();
        create_info.subresourceRange.aspectMask = format.vk_aspect();
        create_info.subresourceRange.layerCount = layers;
        create_info.subresourceRange.levelCount = mips;
    }

    VkHandle<VkImageView> view;
    vk_check(vkCreateImageView(vk_device(), &create_info, vk_allocation_callbacks(), view.get_ptr_for_init()));
    return view;
}

static std::tuple<VkHandle<VkImage>, DeviceMemory, VkHandle<VkImageView>> alloc_image(const math::Vec3ui& size, u32 layers, u32 mips, ImageFormat format, ImageUsage usage, ImageType type) {
    y_profile();

    auto image = create_image(size, layers, mips, format, usage, type);
    auto memory = device_allocator().alloc(image);
    bind_image_memory(image, memory);

    return {std::move(image), std::move(memory), create_view(image, format, layers, mips, type)};
}

static void upload_data(ImageBase& image, const ImageData& data) {
    y_profile();

    const auto staging_buffer = create_stagin_buffer(data.byte_size(), data.data());
    const auto regions = get_copy_regions(data);

    TransferCmdBufferRecorder recorder = create_disposable_transfer_cmd_buffer();

    {
        const auto region = recorder.region("Image upload");
        recorder.barriers({ImageBarrier::transition_barrier(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)});
        vkCmdCopyBufferToImage(recorder.vk_cmd_buffer(), staging_buffer.vk_buffer(), image.vk_image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, u32(regions.size()), regions.data());
        recorder.barriers({ImageBarrier::transition_barrier(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vk_image_layout(image.usage()))});
    }

    loading_command_queue().submit_async_start(std::move(recorder));
}

static void transition_image(ImageBase& image) {
    y_profile();

    TransferCmdBufferRecorder recorder = create_disposable_transfer_cmd_buffer();
    recorder.barriers({ImageBarrier::transition_barrier(image, VK_IMAGE_LAYOUT_UNDEFINED, vk_image_layout(image.usage()))});
    loading_command_queue().submit_async_start(std::move(recorder));
}

static void check_layer_count(ImageType type, const math::Vec3ui& size, usize layers) {
    if(type == ImageType::TwoD && layers > 1) {
        y_fatal("Invalid layer count");
    }
    if(type == ImageType::Cube && layers != 6) {
        y_fatal("Invalid layer count");
    }
    if(size.z() == 0) {
        y_fatal("Invalid size");
    }
    if(size.z() != 1 && type != ImageType::ThreeD) {
        y_fatal("Invalid size");
    }
}


ImageBase::ImageBase(ImageFormat format, ImageUsage usage, const math::Vec3ui& size, ImageType type, usize layers, usize mips) :
        _size(size),
        _layers(u32(layers)),
        _mips(u32(mips)),
        _format(format),
        _usage(usage) {

    check_layer_count(type, _size, _layers);

    std::tie(_image, _memory, _view) = alloc_image(_size, _layers, _mips, _format, _usage, type);

    transition_image(*this);
}

ImageBase::ImageBase(ImageUsage usage, ImageType type, const ImageData& data) :
        _size(data.size()),
        _mips(u32(data.mipmaps())),
        _format(data.format()),
        _usage(usage | ImageUsage::TransferDstBit) {

    check_layer_count(type, _size, _layers);

    std::tie(_image, _memory, _view) = alloc_image(_size, _layers, _mips, _format, _usage, type);

    upload_data(*this, data);
}

ImageBase::~ImageBase() {
    destroy_graphic_resource(std::move(_view));
    destroy_graphic_resource(std::move(_image));
    destroy_graphic_resource(std::move(_memory));
}

bool ImageBase::is_null() const {
    return !_image;
}

const math::Vec3ui& ImageBase::image_size() const {
    return _size;
}

usize ImageBase::mipmaps() const {
    return _mips;
}

usize ImageBase::layers() const {
    return _layers;
}

ImageFormat ImageBase::format() const {
    return _format;
}

ImageUsage ImageBase::usage() const {
    return _usage;
}

VkImageView ImageBase::vk_view() const {
    y_debug_assert(!is_null());
    return _view;
}

VkImage ImageBase::vk_image() const {
    y_debug_assert(!is_null());
    return _image;
}

const DeviceMemory& ImageBase::device_memory() const {
    return _memory;
}

}

