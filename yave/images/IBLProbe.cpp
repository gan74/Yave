/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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

#include <yave/shaders/ComputeProgram.h>
#include <yave/bindings/DescriptorSet.h>
#include <yave/commands/CmdBufferRecorder.h>

#include <y/core/Chrono.h>
#include <y/io/File.h>

namespace yave {

static vk::ImageView create_view(DevicePtr dptr, vk::Image image, ImageFormat format, usize start_mip) {
	return dptr->vk_device().createImageView(vk::ImageViewCreateInfo()
			.setImage(image)
			.setViewType(vk::ImageViewType::eCube)
			.setFormat(format.vk_format())
			.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(format.vk_aspect())
					.setBaseArrayLayer(0)
					.setLayerCount(6)
					.setBaseMipLevel(start_mip)
					.setLevelCount(1)
				)
		);
}

static usize mipmap_count(usize size, usize min_size) {
	if(size % min_size) {
		fatal("Minimum size does not divide image size.");
	}
	if(size <= min_size) {
		fatal("IBL probe is too small.");
	}
	return 1 + std::floor(std::log2(size / min_size));
}

static ComputeShader create_convolution_shader(DevicePtr dptr, const Cubemap&) {
	return ComputeShader(dptr, SpirVData::from_file(io::File::open("cubemap_convolution.comp.spv").expected("Unable to open SPIR-V file.")));
}

static ComputeShader create_convolution_shader(DevicePtr dptr, const Texture&) {
	return ComputeShader(dptr, SpirVData::from_file(io::File::open("equirec_convolution.comp.spv").expected("Unable to open SPIR-V file.")));
}

static constexpr usize diffuse_size = 32;

using ViewBase = ImageView<ImageUsage::ColorBit | ImageUsage::StorageBit, ImageType::Cube>;

struct ProbeBase : ImageBase {
	ProbeBase(DevicePtr dptr, usize size) :
		ImageBase(dptr, vk::Format::eR8G8B8A8Unorm, ImageUsage::TextureBit | ImageUsage::StorageBit, {size, size}, ImageType::Cube, 6, mipmap_count(size, diffuse_size)) {
	}
};

struct ProbeBaseView : ViewBase {
	// does not destroy the view, need to be done manually
	ProbeBaseView(ProbeBase& base, usize mip) :
			ViewBase(base.device(),
					 base.size() / (1 << mip),
					 base.usage(),
					 base.format(),
					 create_view(base.device(), base.vk_image(), base.format(), mip)) {
	}
};

template<ImageType T>
static void fill_probe(const core::ArrayView<ViewBase>& views, const Image<ImageUsage::TextureBit, T>& texture) {
	DevicePtr dptr = texture.device();

	auto descriptor_sets = core::vector_with_capacity<DescriptorSet>(views.size());
	std::transform(views.begin(), views.end(), std::back_inserter(descriptor_sets), [&](const CubemapStorageView& view) {
			return DescriptorSet(dptr, {Binding(texture), Binding(view)});
		});


	ComputeProgram conv_program(create_convolution_shader(dptr, texture));
	CmdBufferRecorder recorder = dptr->create_disposable_cmd_buffer();

	float roughness = 0.0f;
	float roughness_step = 1.0f / (views.size() - 1);

	math::Vec2ui size = views[0].size();
	for(usize i = 0; i != views.size(); ++i) {
		recorder.dispatch_size(conv_program, size, {descriptor_sets[i]}, roughness);

		roughness += roughness_step;
		size /= 2;
	}

	dptr->queue(QueueFamily::Graphics).submit<SyncSubmit>(RecordedCmdBuffer(std::move(recorder)));
}

template<ImageType T>
static void compute_probe(ProbeBase& probe, const Image<ImageUsage::TextureBit, T>& texture) {
	DevicePtr dptr = texture.device();

	if(probe.mipmaps() == 1) {
		fatal("IBL probe is too small.");
	}

	// we need to store the views and use sync compute so we don't delete them while they are still in use
	auto mip_views = core::vector_with_capacity<ViewBase>(probe.mipmaps());
	for(usize i = 0; i != probe.mipmaps(); ++i) {
		mip_views << ProbeBaseView(probe, i);
	}

	fill_probe(mip_views, texture);

	for(const auto& v : mip_views) {
		dptr->destroy(v.vk_view());
	}
}


static usize probe_size(const Cubemap& cube) {
	return usize(1) << log2ui(cube.size().x());
}

static usize probe_size(const Texture& tex) {
	const auto& size = tex.size();
	usize face = (size.x() * size.y()) / 6;
	return usize(1) << usize(std::ceil(std::log2(std::sqrt(face))));
}

IBLProbe IBLProbe::from_cubemap(const Cubemap& cube) {
	core::DebugTimer _("IBLProbe::from_cubemap()");

	ProbeBase probe(cube.device(), probe_size(cube));
	compute_probe(probe, cube);
	IBLProbe final;
	final.swap(probe);
	return final;
}

IBLProbe IBLProbe::from_equirec(const Texture& equirec) {
	core::DebugTimer _("IBLProbe::from_equirec()");

	ProbeBase probe(equirec.device(), probe_size(equirec));
	compute_probe(probe, equirec);
	IBLProbe final;
	final.swap(probe);
	return final;
}

IBLProbe::IBLProbe(IBLProbe&& other) {
	swap(other);
}

IBLProbe& IBLProbe::operator=(IBLProbe&& other) {
	swap(other);
	return *this;
}

}
