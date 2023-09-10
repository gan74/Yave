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

#include "IBLProbe.h"
#include "ImageData.h"

#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/graphics/descriptors/DescriptorSet.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdQueue.h>

#include <y/core/ScratchPad.h>
#include <y/core/Chrono.h>

namespace yave {

static VkHandle<VkImageView> create_view(VkImage image, ImageFormat format, usize start_mip) {
    VkImageViewCreateInfo create_info = vk_struct();
    {
        create_info.image = image;
        create_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        create_info.format = format.vk_format();
        create_info.subresourceRange.aspectMask = format.vk_aspect();
        create_info.subresourceRange.layerCount = 6;
        create_info.subresourceRange.baseMipLevel = u32(start_mip);
        create_info.subresourceRange.levelCount = 1;
    }

    VkHandle<VkImageView> view;
    vk_check(vkCreateImageView(vk_device(), &create_info, vk_allocation_callbacks(), view.get_ptr_for_init()));
    return view;
}

static constexpr usize min_face_size = 8;

static usize mipmap_count(usize size) {
    if(size % min_face_size) {
        y_fatal("Minimum size does not divide image size.");
    }
    if(size <= min_face_size) {
        y_fatal("IBL probe is too small.");
    }
    return 1 + usize(std::floor(std::log2(size / min_face_size)));
}



using ViewBase = ImageView<ImageUsage::ColorBit | ImageUsage::StorageBit, ImageType::Cube>;

struct ProbeBase : ImageBase {
    ProbeBase(usize size, ImageFormat format = VK_FORMAT_R8G8B8A8_UNORM) :
        ImageBase(format, ImageUsage::TextureBit | ImageUsage::StorageBit, {size, size, 1}, ImageType::Cube, 6, mipmap_count(size)) {
    }
};

class ViewContainer : NonMovable {
    public:
        ViewContainer(ProbeBase& base, usize mip) : _view_handle(create_view(base.vk_image(), base.format(), mip)) {
        }

    protected:
        VkHandle<VkImageView> _view_handle;
};

struct ProbeBaseView : ViewContainer, ViewBase {
    ProbeBaseView(ProbeBase& base, usize mip) :
            ViewContainer(base, mip),
            ViewBase(base.image_size().to<2>() / (1 << mip),
                     base.usage(),
                     base.format(),
                     _view_handle,
                     base.vk_image()) {
    }

    ~ProbeBaseView() {
        // Unlike "normal" image views we own the vulkan object
        y_debug_assert(_view_handle == vk_view());
        destroy_graphic_resource(std::move(_view_handle));
    }
};

static const ComputeProgram& convolution_program(const Cubemap&) {
    return device_resources()[DeviceResources::CubemapConvolutionProgram];
}

static const ComputeProgram& convolution_program(const Texture&) {
    return device_resources()[DeviceResources::EquirecConvolutionProgram];
}


template<ImageType T>
static void fill_probe(core::MutableSpan<ProbeBaseView> views, const Image<ImageUsage::TextureBit, T>& texture) {
    const ComputeProgram& conv_program = convolution_program(texture);
    CmdBufferRecorder recorder = create_disposable_cmd_buffer();

    const float roughness_step = 1.0f / (views.size() - 1);

    math::Vec2ui size = views[0].size();
    {
        const auto region = recorder.region("IBL probe generation");
        for(usize i = 0; i != views.size(); ++i) {
            ImageView<ImageUsage::StorageBit, ImageType::Cube> z = views[i];
            const float roughness = (i * roughness_step);
            const auto ds = DescriptorSet(
                Descriptor(texture, SamplerType::LinearClamp),
                z,
                InlineDescriptor(roughness)
            );
            recorder.dispatch_size(conv_program, size, ds);

            size /= 2;
        }
    }

    recorder.submit();
}

template<ImageType T>
static void compute_probe(ProbeBase& probe, const Image<ImageUsage::TextureBit, T>& texture) {
    y_profile();

    if(probe.mipmaps() == 1) {
        y_fatal("IBL probe is too small.");
    }

    auto mip_views = core::ScratchVector<ProbeBaseView>(probe.mipmaps());
    for(usize i = 0; i != probe.mipmaps(); ++i) {
        mip_views.emplace_back(probe, i);
    }

    fill_probe(mip_views, texture);
}


static usize probe_size(const Cubemap& cube) {
    return std::max(min_face_size * 2, 1_uu << log2ui(cube.size().x()));
}

static usize probe_size(const Texture& tex) {
    const auto& size = tex.size();
    const usize face = (size.x() * size.y()) / 6;
    return std::max(min_face_size * 2, 1_uu << usize(std::ceil(std::log2(std::sqrt(face)))));
}

static ImageFormat probe_format(const ImageFormat& input_format) {
    return input_format.is_sRGB()
        ? VK_FORMAT_R8G8B8A8_SRGB
        : VK_FORMAT_R8G8B8A8_UNORM;
}


IBLProbe::IBLProbe(const ImageData& data) {
    *this = from_equirec(Texture(data));
}


IBLProbe IBLProbe::from_cubemap(const Cubemap& cube) {
    y_profile();

    ProbeBase probe(probe_size(cube), probe_format(cube.format()));
    compute_probe(probe, cube);

    IBLProbe final;
    final.ImageBase::operator=(std::move(probe));
    return final;
}

IBLProbe IBLProbe::from_equirec(const Texture& equirec) {
    y_profile();

    ProbeBase probe(probe_size(equirec), probe_format(equirec.format()));
    compute_probe(probe, equirec);

    IBLProbe final;
    final.ImageBase::operator=(std::move(probe));
    return final;
}


}

