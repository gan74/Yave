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
#include <yave/framegraph/FrameGraph.h>

#include <yave/ecs/EntityWorld.h>

#include <yave/components/PointLightComponent.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/entities/entities.h>

#include <y/core/Chrono.h>
#include <y/io2/File.h>

namespace yave {

static Texture create_ibl_lut(DevicePtr dptr, usize size = 512) {
	y_profile();
	core::DebugTimer _("IBLData::create_ibl_lut()");

	const ComputeProgram& brdf_integrator = dptr->device_resources()[DeviceResources::BRDFIntegratorProgram];

	StorageTexture image(dptr, ImageFormat(vk::Format::eR16G16Unorm), {size, size});

	DescriptorSet dset(dptr, {Descriptor(StorageView(image))});

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

static constexpr usize max_directional_light_count = 16;
static constexpr usize max_point_light_count = 1024;

LightingPass LightingPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, const std::shared_ptr<IBLData>& ibl_data) {
	static constexpr vk::Format lighting_format = vk::Format::eR16G16B16A16Sfloat;
	math::Vec2ui size = framegraph.image_size(gbuffer.depth);

	const SceneView& scene = gbuffer.scene_pass.scene_view;


	FrameGraphPassBuilder ambient_builder = framegraph.add_pass("Ambient/Sun pass");
	FrameGraphPassBuilder point_builder = framegraph.add_pass("Lighting pass");

	auto lit = ambient_builder.declare_image(lighting_format, size);

	struct PushData {
		uniform::LightingCamera camera;
		u32 light_count = 0;
	};

	uniform::LightingCamera camera_data = scene.camera();

	{
		auto directional_buffer = ambient_builder.declare_typed_buffer<uniform::DirectionalLight>(max_directional_light_count);

		ambient_builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::ComputeBit);
		ambient_builder.add_uniform_input(gbuffer.color, 0, PipelineStage::ComputeBit);
		ambient_builder.add_uniform_input(gbuffer.normal, 0, PipelineStage::ComputeBit);
		ambient_builder.add_uniform_input(ibl_data->envmap(), 0, PipelineStage::ComputeBit);
		ambient_builder.add_uniform_input(ibl_data->brdf_lut(), 0, PipelineStage::ComputeBit);
		ambient_builder.add_storage_input(directional_buffer, 0, PipelineStage::ComputeBit);
		ambient_builder.add_storage_output(lit, 0, PipelineStage::ComputeBit);
		ambient_builder.map_update(directional_buffer);

		ambient_builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
				PushData push_data{camera_data, 0};
				TypedMapping<uniform::DirectionalLight> mapping = self->resources()->mapped_buffer(directional_buffer);
				for(auto [l] : scene.world().view(DirectionalLightArchetype()).components()) {
					static_assert(std::is_reference_v<decltype(l)>);
					mapping[push_data.light_count++] = uniform::DirectionalLight{
							-l.direction().normalized(),
							0,
							l.color() * l.intensity(),
							0
						};
				}

				const auto& program = recorder.device()->device_resources()[DeviceResources::DeferredSunProgram];
				recorder.dispatch_size(program, size, {self->descriptor_sets()[0]}, push_data);
			});
	}


	{
		auto light_buffer = point_builder.declare_typed_buffer<uniform::PointLight>(max_point_light_count);

		point_builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::ComputeBit);
		point_builder.add_uniform_input(gbuffer.color, 0, PipelineStage::ComputeBit);
		point_builder.add_uniform_input(gbuffer.normal, 0, PipelineStage::ComputeBit);
		point_builder.add_storage_input(light_buffer, 0, PipelineStage::ComputeBit);
		point_builder.add_storage_output(lit, 0, PipelineStage::ComputeBit);
		point_builder.map_update(light_buffer);

		point_builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
				PushData push_data{camera_data, 0};
				TypedMapping<uniform::PointLight> mapping = self->resources()->mapped_buffer(light_buffer);
				for(auto [t, l] : scene.world().view(PointLightArchetype()).components()) {
					static_assert(std::is_reference_v<decltype(t)> && std::is_reference_v<decltype(l)>);
					mapping[push_data.light_count++] = uniform::PointLight{
							t.position(),
							l.radius(),
							l.color() * l.intensity(),
							std::max(math::epsilon<float>, l.falloff())
						};
				}


				const auto& program = recorder.device()->device_resources()[DeviceResources::DeferredLocalsProgram];
				recorder.dispatch_size(program, size, {self->descriptor_sets()[0]}, push_data);
			});
	}

	LightingPass pass;
	pass.lit = lit;
	return pass;
}

}
