/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include "LightingPass.h"

#include <yave/device/Device.h>
#include <yave/components/LightComponent.h>
#include <yave/ecs/EntityWorld.h>

#include <y/core/Chrono.h>
#include <y/io2/File.h>

namespace yave {

static Texture create_ibl_lut(DevicePtr dptr, usize size = 512) {
	y_profile();
	core::DebugTimer _("IBLData::create_ibl_lut()");

	const ComputeProgram& brdf_integrator = dptr->device_resources()[DeviceResources::BRDFIntegratorProgram];

	StorageTexture image(dptr, ImageFormat(vk::Format::eR16G16Unorm), {size, size});

	DescriptorSet dset(dptr, {Binding(StorageView(image))});

	CmdBufferRecorder recorder = dptr->create_disposable_cmd_buffer();
	{
		auto region = recorder.region("IBLData::create_ibl_lut");
		recorder.dispatch_size(brdf_integrator, image.size(), {dset});
	}
	dptr->graphic_queue().submit<SyncSubmit>(RecordedCmdBuffer(std::move(recorder)));

	return image;
}

static auto load_envmap() {
	if(auto file = io2::File::open("equirec.yt")) {
		ImageData data;
		serde2::ReadableArchive ar(file.unwrap());
		if(data.deserialize(ar)) {
			return data;
		}
		log_msg("Unable to read envmap texture.", Log::Error);
	} else {
		log_msg("Unable to open envmap file.", Log::Error);
	}

	math::Vec4ui data(0xFFFFFFFF);
	return ImageData(math::Vec2ui(2), reinterpret_cast<const u8*>(data.data()), vk::Format::eR8G8B8A8Unorm);
}

IBLData::IBLData(DevicePtr dptr) : IBLData(dptr, load_envmap()) {
}

IBLData::IBLData(DevicePtr dptr, const ImageData& envmap_data) :
		DeviceLinked(dptr),
		_brdf_lut(create_ibl_lut(dptr)),
		_envmap(IBLProbe::from_equirec(Texture(dptr, envmap_data))) {
}


const IBLProbe& IBLData::envmap() const {
	return _envmap;
}

TextureView IBLData::brdf_lut() const  {
	return _brdf_lut;
}


static constexpr usize max_light_count = 1024;

LightingPass LightingPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, const std::shared_ptr<IBLData>& ibl_data) {
	y_profile();

	static constexpr vk::Format lighting_format = vk::Format::eR16G16B16A16Sfloat;
	math::Vec2ui size = framegraph.image_size(gbuffer.depth);

	const SceneView& scene = gbuffer.scene_pass.scene_view;

	auto lit = framegraph.declare_image(lighting_format, size);
	auto light_buffer = framegraph.declare_typed_buffer<uniform::Light>(max_light_count);

	LightingPass pass;
	pass.lit = lit;

	FrameGraphPassBuilder builder = framegraph.add_pass("Lighting pass");
	builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(gbuffer.color, 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(gbuffer.normal, 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(ibl_data->envmap(), 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(ibl_data->brdf_lut(), 0, PipelineStage::ComputeBit);
	builder.add_storage_input(light_buffer, 0, PipelineStage::ComputeBit);
	builder.add_storage_output(lit, 0, PipelineStage::ComputeBit);

	builder.map_update(light_buffer);

	builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			struct CameraData {
				math::Matrix4<> inv_matrix;
				math::Vec3 position;
				u32 padding_0 = 0;
				math::Vec3 forward;
				u32 padding_1 = 0;
			};
			struct PushData {
				CameraData camera;
				u32 point_count = 0;
				u32 directional_count = 0;
			} push_data;

			const Camera& camera = scene.camera();
			push_data.camera.inv_matrix = camera.inverse_matrix();
			push_data.camera.position = camera.position();
			push_data.camera.forward = camera.forward();


			TypedMapping<uniform::Light> mapping = self->resources()->mapped_buffer(light_buffer);

			Y_TODO(We should not need this)
			{
				if(scene.has_scene()) {
					usize light_count = scene.scene().lights().size();
					for(const auto& l : scene.scene().lights()) {
						if(l->type() == Light::Point) {
							mapping[push_data.point_count++] = *l;
						} else {
							mapping[light_count - ++push_data.directional_count] = *l;
						}
					}
				}
				if(scene.has_world()) {
					const auto& lights = scene.world().components<LightComponent>();
					usize light_count = lights.size();
					for(const auto& l : lights) {
						if(l.light().type() == Light::Point) {
							mapping[push_data.point_count++] = l.light();
						} else {
							mapping[light_count - ++push_data.directional_count] = l.light();
						}
					}
				}
			}

			const auto& program = recorder.device()->device_resources()[DeviceResources::DeferredLightingProgram];
			recorder.dispatch_size(program, size, {self->descriptor_sets()[0]}, push_data);
		});

	return pass;
}

}
