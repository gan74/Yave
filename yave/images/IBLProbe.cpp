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

static ComputeShader create_convolution_shader(DevicePtr dptr) {
	return ComputeShader(dptr, SpirVData::from_file(io::File::open("ibl_convolution.comp.spv").expected("Unable to open SPIR-V file.")));
}

static usize assert_square(const math::Vec2ui& size) {
	if(size.x() != size.y()) {
		fatal("Cubemap is not square.");
	}
	if(usize(1) << log2ui(size.x()) != size.x()) {
		fatal("Cubemap size is not a power of 2.");
	}
	return size.x();
}

static usize mipmap_count(usize size, usize group_size) {
	if(size % group_size) {
		fatal("Group size does not divide image size.");
	}
	return 1 + std::floor(std::log2(size / group_size));
}

IBLProbe IBLProbe::from_cubemap(const Cubemap& cube) {
	core::DebugTimer _("IBLProbe::from_cubemap()");

	struct ProbeBase : ImageBase {
		ProbeBase(DevicePtr dptr, usize size, usize conv_size) :
			ImageBase(dptr, vk::Format::eR8G8B8A8Unorm, ImageUsage::TextureBit | ImageUsage::StorageBit, {size, size}, ImageType::Cube, 6, mipmap_count(size, conv_size)) {
		}
	};

	using ViewBase = ImageView<ImageUsage::ColorBit | ImageUsage::StorageBit, ImageType::Cube>;

	// ImageView implement size as _image->size() so it'll be wrong for this class and shoudl'nt be used
	struct ProbeBaseView : ViewBase {
		// does not destroy the view, need to be done manually
		ProbeBaseView(ProbeBase& base, usize mip) : ViewBase(&base, create_view(base.device(), base.vk_image(), base.format(), mip)) {
		}
	};

	DevicePtr dptr = cube.device();
	ComputeProgram conv_program(create_convolution_shader(dptr));

	usize conv_size = assert_square(conv_program.local_size().to<2>());
	ProbeBase probe(dptr, 1 << log2ui(assert_square(cube.size())), conv_size);

	// we need to store the views and use sync compute so we don't delete them while they are still in use
	auto mip_views = core::vector_with_capacity<ProbeBaseView>(probe.mipmaps());
	for(usize i = 0; i != probe.mipmaps(); ++i) {
		mip_views << ProbeBaseView(probe, i);
	}

	auto descriptor_sets = core::vector_with_capacity<DescriptorSet>(probe.mipmaps());
	std::transform(mip_views.begin(), mip_views.end(), std::back_inserter(descriptor_sets), [&](const CubemapStorageView& view) {
			return DescriptorSet(dptr, {Binding(cube), Binding(view)});
		});


	CmdBufferRecorder recorder = dptr->create_disposable_cmd_buffer();
	for(usize i = 0; i != probe.mipmaps(); ++i) {
		auto mip_size = probe.size() / (1 << i);
		recorder.dispatch_size(conv_program, mip_size, {descriptor_sets[i]});
	}
	RecordedCmdBuffer(std::move(recorder)).submit<SyncSubmit>(dptr->vk_queue(QueueFamily::Graphics));


	for(const auto& v : mip_views) {
		dptr->destroy(v.vk_view());
	}

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
