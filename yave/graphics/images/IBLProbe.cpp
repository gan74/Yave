/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/device/DeviceResources.h>

#include <y/core/Chrono.h>

namespace yave {

static VkImageView create_view(VkImage image, ImageFormat format, usize start_mip) {
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

    VkImageView view = {};
    vk_check(vkCreateImageView(vk_device(), &create_info, vk_allocation_callbacks(), &view));
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

struct ProbeBaseView : ViewBase {
    // does not destroy the view, need to be done manually
    ProbeBaseView(ProbeBase& base, usize mip) :
            ViewBase(base.image_size().to<2>() / (1 << mip),
                     base.usage(),
                     base.format(),
                     create_view(base.vk_image(), base.format(), mip),
                     base.vk_image()) {
    }
};

static const ComputeProgram& convolution_program(const Cubemap&) {
    return device_resources()[DeviceResources::CubemapConvolutionProgram];
}

static const ComputeProgram& convolution_program(const Texture&) {
    return device_resources()[DeviceResources::EquirecConvolutionProgram];
}


template<ImageType T>
static void fill_probe(core::Span<ViewBase> views, const Image<ImageUsage::TextureBit, T>& texture) {
    auto descriptor_sets = core::vector_with_capacity<DescriptorSet>(views.size());
    std::transform(views.begin(), views.end(), std::back_inserter(descriptor_sets), [&](const CubemapStorageView& view) {
            return DescriptorSet({Descriptor(texture, SamplerType::LinearClamp), Descriptor(view)});
        });


    const ComputeProgram& conv_program = convolution_program(texture);
    CmdBufferRecorder recorder = create_disposable_cmd_buffer();

    const float roughness_step = 1.0f / (views.size() - 1);

    math::Vec2ui size = views[0].size();
    {
        const auto region = recorder.region("IBL probe generation");
        for(usize i = 0; i != views.size(); ++i) {
            const float roughness = (i * roughness_step);
            recorder.dispatch_size(conv_program, size, {descriptor_sets[i]}, roughness);

            size /= 2;
        }
    }

    // use sync compute to avoid having to sync later
    std::move(recorder).submit<SyncPolicy::Sync>();
}

template<ImageType T>
static void compute_probe(ProbeBase& probe, const Image<ImageUsage::TextureBit, T>& texture) {
    y_profile();

    if(probe.mipmaps() == 1) {
        y_fatal("IBL probe is too small.");
    }

    auto mip_views = core::vector_with_capacity<ViewBase>(probe.mipmaps());
    for(usize i = 0; i != probe.mipmaps(); ++i) {
        mip_views << ProbeBaseView(probe, i);
    }

    fill_probe(mip_views, texture);

    for(const auto& v : mip_views) {
        device_destroy(v.vk_view());
    }
}


static usize probe_size(const Cubemap& cube) {
    return std::max(min_face_size * 2, usize(1) << log2ui(cube.size().x()));
}

static usize probe_size(const Texture& tex) {
    const auto& size = tex.size();
    const usize face = (size.x() * size.y()) / 6;
    return std::max(min_face_size * 2, usize(1) << usize(std::ceil(std::log2(std::sqrt(face)))));
}






IBLProbe::IBLProbe(const ImageData& data) {
    *this = data.layers() == 6
        ? from_cubemap(Cubemap(data))
        : from_equirec(Texture(data));
}


IBLProbe IBLProbe::from_cubemap(const Cubemap& cube) {
    core::DebugTimer _("IBLProbe::from_cubemap()");

    ProbeBase probe(probe_size(cube), cube.format());
    compute_probe(probe, cube);

    IBLProbe final;
    final.ImageBase::operator=(std::move(probe));
    return final;
}

IBLProbe IBLProbe::from_equirec(const Texture& equirec) {
    core::DebugTimer _("IBLProbe::from_equirec()");

    ProbeBase probe(probe_size(equirec), equirec.format());
    compute_probe(probe, equirec);

    IBLProbe final;
    final.ImageBase::operator=(std::move(probe));
    return final;
}


}

