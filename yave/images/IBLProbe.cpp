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

static void convolution(ImageView<ImageUsage::StorageBit, ImageType::Cube>&& probe, const Cubemap& cube) {
	DevicePtr dptr = probe.device();

	ComputeProgram program(create_convolution_shader(dptr));
	DescriptorSet descriptor_set(dptr, {Binding(cube), Binding(probe)});

	CmdBufferRecorder recorder = dptr->create_disposable_cmd_buffer();
	recorder.dispatch(program, math::Vec3ui{probe.size(), 1}, {descriptor_set});

	// we need to use sync compute so we don't delete view while it is being used
	RecordedCmdBuffer(std::move(recorder)).submit<SyncSubmit>(dptr->vk_queue(QueueFamily::Graphics));
}

static mipmap_count(const math::Vec2ui& size) {
	if(size.x() != size.y()) {
		fatal("Cubemap is not square");
	}
	if(size.x() < 512) {
		fatal("Cubemap is too small");
	}
	return 8;
}


IBLProbe IBLProbe::from_cubemap(const Cubemap& cube) {
	struct ProbeBase : ImageBase {
		ProbeBase(DevicePtr dptr, const math::Vec2ui& size, usize mips) :
			ImageBase(dptr, vk::Format::eR8G8B8A8Unorm, ImageUsage::TextureBit | ImageUsage::StorageBit, size, ImageType::Cube, 6, mips) {
		}
	};

	using ViewBase = ImageView<ImageUsage::ColorBit | ImageUsage::StorageBit, ImageType::Cube>;

	// ImageView implement size as _image->size() so it'll be wrong for this class and shoudl'nt be used
	struct ProbeBaseView : ViewBase {
		ProbeBaseView(ProbeBase& base, usize mip) : ViewBase(&base, create_view(base.device(), base.vk_image(), base.format(), mip)) {
		}

		~ProbeBaseView() {
			device()->destroy(vk_view());
		}
	};

	ProbeBase base(cube.device(), cube.size(), mipmap_count(cube.size()));

	for(usize i = 0; i != base.mipmaps(); ++i) {
		convolution(ProbeBaseView(base, i), cube);
		log_msg("elel");
	}




	IBLProbe probe;
	probe.swap(base);
	return probe;
}






IBLProbe::IBLProbe(IBLProbe&& other) {
	swap(other);
}

IBLProbe& IBLProbe::operator=(IBLProbe&& other) {
	swap(other);
	return *this;
}

}
