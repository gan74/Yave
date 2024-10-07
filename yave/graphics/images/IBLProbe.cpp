/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include <yave/framegraph/TransientMipView.h>

#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/graphics/descriptors/DescriptorSet.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdQueue.h>

#include <y/core/ScratchPad.h>
#include <y/core/Chrono.h>

namespace yave {

static constexpr u32 min_face_size = 8;

static u32 mipmap_count(u32 size) {
    if(size % min_face_size) {
        y_fatal("Minimum size does not divide image size");
    }
    if(size <= min_face_size) {
        y_fatal("IBL probe is too small");
    }
    return 1 + u32(std::floor(std::log2(size / min_face_size)));
}

using ProbeBase = Image<ImageUsage::TextureBit | ImageUsage::StorageBit, ImageType::Cube>;
using ProbeMipView = TransientMipView<ImageUsage::TextureBit | ImageUsage::StorageBit, ImageType::Layered>;

static const ComputeProgram& convolution_program(const Cubemap&) {
    return device_resources()[DeviceResources::CubemapConvolutionProgram];
}

static const ComputeProgram& convolution_program(const Texture&) {
    return device_resources()[DeviceResources::EquirecConvolutionProgram];
}

template<ImageType T>
static void fill_probe(core::MutableSpan<ProbeMipView> views, const Image<ImageUsage::TextureBit, T>& texture) {
    const ComputeProgram& conv_program = convolution_program(texture);
    CmdBufferRecorder recorder = create_disposable_cmd_buffer();

    const float roughness_step = 1.0f / (views.size() - 1);

    {
        const auto region = recorder.region("IBL probe generation");
        for(usize i = 0; i != views.size(); ++i) {
            ImageView<ImageUsage::StorageBit, ImageType::Layered> out_view = views[i];
            y_debug_assert(out_view.size().x() == out_view.size().y());
            const float roughness = (i * roughness_step);
            const auto ds = DescriptorSet(
                Descriptor(texture, SamplerType::LinearClamp),
                out_view,
                InlineDescriptor(math::Vec2(roughness, out_view.size().x()))
            );
            recorder.dispatch_threads(conv_program, math::Vec3ui(out_view.size(), 6), ds);
        }
    }

    recorder.submit();
}

template<ImageType T>
static void compute_probe(ProbeBase& probe, const Image<ImageUsage::TextureBit, T>& texture) {
    y_profile();

    if(probe.mipmaps() == 1) {
        y_fatal("IBL probe is too small");
    }

    auto mip_views = core::ScratchVector<ProbeMipView>(probe.mipmaps());
    for(u32 i = 0; i != probe.mipmaps(); ++i) {
        mip_views.emplace_back(probe, i);
    }

    fill_probe(mip_views, texture);
}


static u32 probe_size(const Cubemap& cube) {
    return std::max(min_face_size * 2, 1u << log2ui(cube.size().x()));
}

static u32 probe_size(const Texture& tex) {
    const math::Vec2ui size = tex.size();
    const u32 face = (size.x() * size.y()) / 6;
    return std::max(min_face_size * 2, 1u << u32(std::ceil(std::log2(std::sqrt(face)))));
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

    const u32 size = probe_size(cube);
    ProbeBase probe(probe_format(cube.format()), math::Vec2ui(size), mipmap_count(size));
    compute_probe(probe, cube);

    IBLProbe final;
    final.ImageBase::operator=(std::move(probe));
    return final;
}

IBLProbe IBLProbe::from_equirec(const Texture& equirec) {
    y_profile();

    const u32 size = probe_size(equirec);
    ProbeBase probe(probe_format(equirec.format()), math::Vec2ui(size), mipmap_count(size));
    compute_probe(probe, equirec);

    IBLProbe final;
    final.ImageBase::operator=(std::move(probe));
    return final;
}

}

