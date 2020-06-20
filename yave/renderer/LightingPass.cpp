/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#include <yave/components/SpotLightComponent.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/SkyLightComponent.h>
#include <yave/entities/entities.h>

#include <yave/meshes/StaticMesh.h>


namespace yave {

static constexpr ImageFormat lighting_format = VK_FORMAT_R16G16B16A16_SFLOAT;
static constexpr usize max_directional_lights = 16;
static constexpr usize max_point_lights = 1024;
static constexpr usize max_spot_lights = 1024;
static constexpr usize max_shadow_lights = 128;


static const IBLProbe* find_probe(DevicePtr dptr, const ecs::EntityWorld& world) {
	for(const SkyLightComponent& sky : world.components<SkyLightComponent>()) {
		if(const IBLProbe* probe = sky.probe().get()) {
			y_debug_assert(probe->device());
			return probe;
		}
	}

	return dptr->device_resources().empty_probe().get();
}

static FrameGraphMutableImageId ambient_pass(FrameGraph& framegraph,
											 const math::Vec2ui& size,
											 const GBufferPass& gbuffer) {

	const SceneView& scene = gbuffer.scene_pass.scene_view;
	const IBLProbe* ibl_probe = find_probe(framegraph.device(), scene.world());

	struct PushData {
		u32 light_count;
	};

	FrameGraphPassBuilder builder = framegraph.add_pass("Ambient/Sun pass");

	const auto lit = builder.declare_image(lighting_format, size);

	const auto directional_buffer = builder.declare_typed_buffer<uniform::DirectionalLight>(max_directional_lights);

	builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(gbuffer.color, 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(gbuffer.normal, 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(*ibl_probe, 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(builder.device()->device_resources().brdf_lut(), 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(gbuffer.scene_pass.camera_buffer, 0, PipelineStage::ComputeBit);
	builder.add_storage_input(directional_buffer, 0, PipelineStage::ComputeBit);
	builder.add_storage_output(lit, 0, PipelineStage::ComputeBit);
	builder.map_update(directional_buffer);

	builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
		PushData push_data{0};
		TypedMapping<uniform::DirectionalLight> mapping = self->resources().mapped_buffer(directional_buffer);
		for(auto [l] : scene.world().view(DirectionalLightArchetype()).components()) {
			mapping[push_data.light_count++] = {
					-l.direction().normalized(),
					0,
					l.color() * l.intensity(),
					0
				};
		}

		const auto& program = recorder.device()->device_resources()[DeviceResources::DeferredAmbientProgram];
		recorder.dispatch_size(program, size, {self->descriptor_sets()[0]}, push_data);
	});

	return lit;
}


static void local_lights_pass(FrameGraph& framegraph,
							  FrameGraphMutableImageId lit,
							  const math::Vec2ui& size,
							  const GBufferPass& gbuffer,
							  const ShadowMapPass& shadow_pass) {

	struct PushData {
		u32 point_count = 0;
		u32 spot_count = 0;
		u32 shadow_count = 0;
	};

	const SceneView& scene = gbuffer.scene_pass.scene_view;

	FrameGraphPassBuilder builder = framegraph.add_pass("Lighting pass");

	const auto point_buffer = builder.declare_typed_buffer<uniform::PointLight>(max_point_lights);
	const auto spot_buffer = builder.declare_typed_buffer<uniform::SpotLight>(max_spot_lights);
	const auto shadow_buffer = builder.declare_typed_buffer<uniform::ShadowMapParams>(max_shadow_lights);

	builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(gbuffer.color, 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(gbuffer.normal, 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(shadow_pass.shadow_map, 0, PipelineStage::ComputeBit);
	builder.add_uniform_input(gbuffer.scene_pass.camera_buffer, 0, PipelineStage::ComputeBit);
	builder.add_storage_input(point_buffer, 0, PipelineStage::ComputeBit);
	builder.add_storage_input(spot_buffer, 0, PipelineStage::ComputeBit);
	builder.add_storage_input(shadow_buffer, 0, PipelineStage::ComputeBit);
	builder.add_storage_output(lit, 0, PipelineStage::ComputeBit);

	builder.map_update(point_buffer);
	builder.map_update(spot_buffer);
	builder.map_update(shadow_buffer);

	builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
		PushData push_data{0, 0, 0};

		{
			TypedMapping<uniform::PointLight> mapping = self->resources().mapped_buffer(point_buffer);
			for(auto [t, l] : scene.world().view(PointLightArchetype()).components()) {
				mapping[push_data.point_count++] = {
					t.position(),
					l.radius(),
					l.color() * l.intensity(),
					std::max(math::epsilon<float>, l.falloff())
				};
			}
		}

		{
			TypedMapping<uniform::SpotLight> mapping = self->resources().mapped_buffer(spot_buffer);
			TypedMapping<uniform::ShadowMapParams> shadow_mapping = self->resources().mapped_buffer(shadow_buffer);
			for(auto spot : scene.world().view(SpotLightArchetype())) {
				auto [t, l] = spot.components();

				u32 shadow_index = u32(-1);
				if(l.cast_shadow()) {
					if(const auto it = shadow_pass.sub_passes->lights.find(spot.id().as_u64()); it != shadow_pass.sub_passes->lights.end()) {
						shadow_mapping[shadow_index = push_data.shadow_count++] = it->second;
					}
				}

				mapping[push_data.spot_count++] = {
					t.position(),
					l.radius(),
					l.color() * l.intensity(),
					std::max(math::epsilon<float>, l.falloff()),
					-t.forward(),
					std::cos(l.half_angle()),
					std::max(math::epsilon<float>, l.angle_exponent()),
					shadow_index,
					{}
				};
			}
		}

		if(push_data.point_count || push_data.spot_count) {
			const auto& program = recorder.device()->device_resources()[DeviceResources::DeferredLocalsProgram];
			recorder.dispatch_size(program, size, {self->descriptor_sets()[0]}, push_data);
		}
	});
}


LightingPass LightingPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, const ShadowMapPassSettings& settings) {
	const math::Vec2ui size = framegraph.image_size(gbuffer.depth);
	const SceneView& scene = gbuffer.scene_pass.scene_view;

	LightingPass pass;
	pass.shadow_pass = ShadowMapPass::create(framegraph, scene, settings);

	const auto lit = ambient_pass(framegraph, size, gbuffer);

	local_lights_pass(framegraph, lit, size, gbuffer, pass.shadow_pass);

	pass.lit = lit;
	return pass;
}

}
