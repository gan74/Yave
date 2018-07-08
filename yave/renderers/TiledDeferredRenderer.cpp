/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include "TiledDeferredRenderer.h"

#include <yave/buffers/TypedWrapper.h>
#include <yave/barriers/Barrier.h>

#include <y/io/File.h>
#include <y/core/Chrono.h>

namespace yave {

static ComputeShader create_lighting_shader(DevicePtr dptr) {
	return ComputeShader(dptr, SpirVData::deserialized(io::File::open("deferred.comp.spv").expected("Unable to open SPIR-V file.")));
}

static Texture create_ibl_lut(DevicePtr dptr, usize size = 512) {
	Y_LOG_PERF("deferred,IBL");
	core::DebugTimer _("IBLData::create_ibl_lut()");

	ComputeProgram brdf_integrator(ComputeShader(dptr, SpirVData::deserialized (io::File::open("brdf_integrator.comp.spv").expected("Unable to open SPIR-V file."))));

	StorageTexture image(dptr, ImageFormat(vk::Format::eR16G16Unorm), {size, size});

	DescriptorSet dset(dptr, {Binding(StorageView(image))});

	CmdBufferRecorder recorder = dptr->create_disposable_cmd_buffer();
	{
		auto region = recorder.region("IBLData::create_ibl_lut");
		recorder.dispatch_size(brdf_integrator, image.size(), {dset});
	}
	dptr->queue(QueueFamily::Graphics).submit<SyncSubmit>(RecordedCmdBuffer(std::move(recorder)));

	return image;
}

static auto load_envmap(DevicePtr dptr) {
	auto image = ImageData::deserialized(io::File::open("../tools/image_to_yt/equirec.yt").expected("Unable to open equirec."));
	return IBLProbe::from_equirec(Texture(dptr, image));
}

IBLData::IBLData(DevicePtr dptr) :
		DeviceLinked(dptr),
		_brdf_lut(create_ibl_lut(dptr)),
		_envmap(load_envmap(dptr)) {
}

const IBLProbe& IBLData::envmap() const {
	return _envmap;
}

TextureView IBLData::brdf_lut() const  {
	return _brdf_lut;
}


TiledDeferredRenderer::TiledDeferredRenderer(const Ptr<GBufferRenderer>& gbuffer, const Ptr<IBLData>& ibl_data) :
		Renderer(gbuffer->device()),
		_gbuffer(gbuffer),
		_lighting_program(create_lighting_shader(device())),
		_ibl_data(ibl_data),
		_acc_buffer(device(), ImageFormat(vk::Format::eR16G16B16A16Sfloat), _gbuffer->size()),
		_lights_buffer(device(), max_light_count),
		_descriptor_set(device(), {
				Binding(_gbuffer->depth()),
				Binding(_gbuffer->albedo_metallic()),
				Binding(_gbuffer->normal_roughness()),
				Binding(_ibl_data->envmap()),
				Binding(_ibl_data->brdf_lut()),
				Binding(_lights_buffer),
				Binding(StorageView(_acc_buffer)),
			}) {

	for(usize i = 0; i != 3; i++) {
		if(size()[i] % _lighting_program.local_size()[i]) {
			log_msg("Compute local size at index "_s + i + " does not divide output buffer size.", Log::Warning);
		}
	}
}

const math::Vec2ui& TiledDeferredRenderer::size() const {
	return _acc_buffer.size();
}

TextureView TiledDeferredRenderer::lighting() const {
	return _acc_buffer;
}

TextureView TiledDeferredRenderer::output() const {
	return _acc_buffer;
}

void TiledDeferredRenderer::build_frame_graph(FrameGraphNode& frame_graph) {
	frame_graph.schedule(_gbuffer);
}

void TiledDeferredRenderer::pre_render(CmdBufferRecorder<>& recorder, const FrameToken&) {
	auto region = recorder.region("TiledDeferredRenderer::pre_render");

	auto directionals = core::vector_with_capacity<const Light*>(4);
	auto lights = core::vector_with_capacity<const Light*>(scene_view().scene().lights().size());

	for(const auto& l : scene_view().scene().lights()) {
		(l->type() == Light::Directional ? directionals : lights) << l.get();
	}

	// fill light buffer
	{
		auto mapping = TypedMapping(_lights_buffer);

		#warning cap using max_light_count
		std::transform(lights.begin(), lights.end(), mapping.begin(), [](const Light* l) { return uniform::Light(*l); });
		std::transform(directionals.begin(), directionals.end(), mapping.begin() + lights.size(), [](const Light* l) { return uniform::Light(*l); });
		_directional_count = u32(directionals.size());
	}
}


void TiledDeferredRenderer::render(CmdBufferRecorder<>& recorder, const FrameToken&) {
	auto region = recorder.region("TiledDeferredRenderer::render");

	struct PushData {
		uniform::Camera camera;
		u32 point_count;
		u32 directional_count;
	};

	u32 total_lights = u32(scene_view().scene().lights().size());
	recorder.dispatch_size(
			_lighting_program,
			size(),
			{_descriptor_set},
			PushData{
				_gbuffer->scene_view().camera(),
				u32(total_lights - _directional_count),
				u32(_directional_count)
			}
		);
}

}
