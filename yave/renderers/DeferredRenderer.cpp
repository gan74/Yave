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

#include "DeferredRenderer.h"

#include <yave/images/IBLProbe.h>

#include <yave/commands/CmdBufferRecorder.h>
#include <yave/buffers/CpuVisibleMapping.h>

#include <y/core/Chrono.h>
#include <y/io/File.h>

namespace yave {

static ComputeShader create_lighting_shader(DevicePtr dptr) {
	return ComputeShader(dptr, SpirVData::from_file(io::File::open("deferred.comp.spv").expected("Unable to open SPIR-V file.")));
}

static Texture create_ibl_lut(DevicePtr dptr, usize size = 512) {
	core::DebugTimer _("DeferredRenderer::create_ibl_lut()");

	ComputeProgram brdf_integrator(ComputeShader(dptr, SpirVData::from_file(io::File::open("brdf_integrator.comp.spv").expected("Unable to open SPIR-V file."))));

	StorageTexture image(dptr, ImageFormat(vk::Format::eR16G16Unorm), {size, size});

	DescriptorSet dset(dptr, {Binding(StorageView(image))});

	CmdBufferRecorder recorder = dptr->create_disposable_cmd_buffer();
	recorder.dispatch_size(brdf_integrator, image.size(), {dset});
	RecordedCmdBuffer(std::move(recorder)).submit<SyncSubmit>(dptr->vk_queue(QueueFamily::Graphics));

	return image;
}


static auto load_envmap(DevicePtr dptr) {
	//return Cubemap(dptr, ImageData::from_file(io::File::open("../tools/image_to_yt/cubemap/sky.yt").expected("Unable to open cubemap")));
	return IBLProbe::from_cubemap(Cubemap(dptr, ImageData::from_file(io::File::open("../tools/image_to_yt/cubemap/sky.yt").expected("Unable to open cubemap"))));
}

DeferredRenderer::DeferredRenderer(const Ptr<GBufferRenderer>& gbuffer) :
		BufferRenderer(gbuffer->device()),
		_gbuffer(gbuffer),
		_envmap(load_envmap(device())),
		_lighting_program(create_lighting_shader(device())),
		_ibl_lut(create_ibl_lut(device())),
		_acc_buffer(device(), ImageFormat(vk::Format::eR16G16B16A16Sfloat), _gbuffer->size()),
		_lights_buffer(device(), max_light_count),
		_descriptor_set(device(), {
				Binding(_gbuffer->depth()),
				Binding(_gbuffer->color()),
				Binding(_gbuffer->normal()),
				Binding(_envmap),
				Binding(_ibl_lut),
				Binding(_lights_buffer),
				Binding(StorageView(_acc_buffer))
			}) {

	for(usize i = 0; i != 3; i++) {
		if(size()[i] % _lighting_program.local_size()[i]) {
			log_msg("Compute local size at index "_s + i + " does not divide output buffer size.", Log::Warning);
		}
	}
}

const math::Vec2ui& DeferredRenderer::size() const {
	return _acc_buffer.size();
}

void DeferredRenderer::build_frame_graph(RenderingNode<result_type>& node, CmdBufferRecorder<>& recorder) {
	node.add_dependency(_gbuffer, recorder);
	auto culling = node.add_dependency(_gbuffer->scene_renderer()->culling_node());

	node.set_func([=, &recorder]() -> result_type {
			const auto& directionals = culling.get().directional_lights;
			const auto& lights = culling.get().lights;

			{
				auto mapping = _lights_buffer.map();

				std::transform(lights.begin(), lights.end(), mapping.begin(), [](const Light* l) { return uniform::Light(*l); });
				std::transform(directionals.begin(), directionals.end(), mapping.begin() + lights.size(), [](const Light* l) { return uniform::Light(*l); });
			}

			struct PushData {
				uniform::Camera camera;
				u32 point_count;
				u32 directional_count;
			};

			recorder.dispatch_size(_lighting_program, size(), {_descriptor_set}, PushData{_gbuffer->scene_view().camera(), lights.size(), directionals.size()});
			return _acc_buffer;
		});
}

}
