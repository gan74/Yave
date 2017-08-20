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

static ComputeShader create_convolution_shader(DevicePtr dptr) {
	return ComputeShader(dptr, SpirVData::from_file(io::File::open("ibl_convolution.comp.spv").expected("Unable to open SPIR-V file.")));
}

static void convolution(ImageView<ImageUsage::StorageBit, ImageType::Cube> probe, const Cubemap& cube) {
	DevicePtr dptr = cube.device();

	ComputeProgram program(create_convolution_shader(dptr));
	DescriptorSet descriptor_set(dptr, {Binding(cube), Binding(probe)});

	CmdBufferRecorder recorder = dptr->create_disposable_cmd_buffer();
	recorder.dispatch(program, math::Vec3ui{cube.size(), probe.image().mipmaps()}, {descriptor_set});
	RecordedCmdBuffer(std::move(recorder)).submit<SyncSubmit>(dptr->vk_queue(QueueFamily::Graphics));
}

IBLProbe::IBLProbe(IBLProbe&& other) {
	swap(other);
}

IBLProbe& IBLProbe::operator=(IBLProbe&& other) {
	swap(other);
	return *this;
}

IBLProbe IBLProbe::from_cubemap(const Cubemap& cube) {
	unused(cube);
	return fatal("Not supported.");

	/*struct EmptyProbe : Image<ImageUsage::TextureBit | ImageUsage::StorageBit, ImageType::Cube> {
		EmptyProbe(DevicePtr dptr, const math::Vec2ui& size, usize mips) {
			ProbeBase base(dptr, size, mips);
			swap(base);
		}

		private:
			struct ProbeBase : ImageBase {
				ProbeBase(DevicePtr dptr, const math::Vec2ui& size, usize mips) :
					ImageBase(dptr, vk::Format::eR8G8B8A8Unorm, ImageUsage::TextureBit | ImageUsage::StorageBit, size, ImageType::Cube, 6, mips) {
				}
			};
	};

	EmptyProbe empty(cube.device(), cube.size(), 6);

	convolution(empty, cube);

	IBLProbe probe;
	probe.swap(empty);
	return probe;*/
}


}
