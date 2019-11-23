/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

#include "RayleighSkyPass.h"

#include <yave/framegraph/FrameGraph.h>

#include <yave/ecs/EntityWorld.h>

#include <yave/components/SkyComponent.h>

#include "GBufferPass.h"

namespace yave {

RayleighSkyPass RayleighSkyPass::create(FrameGraph& framegraph, const SceneView& scene_view, FrameGraphImageId in_lit, FrameGraphImageId in_depth, const GBufferPass& gbuffer) {

	RayleighSkyPass pass;
	pass.lit = in_lit;

	FrameGraphMutableImageId lit;

	for(auto [sky] : scene_view.world().view<SkyComponent>().components()) {
		const auto& sun = sky.sun();

		uniform::RayleighSky sky_data {
			scene_view.camera(),
			-sun.direction().normalized(),
			sky.planet_radius() + 100.0f,
			sun.color() * sun.intensity(),
			sky.planet_radius(),
			sky.beta_rayleight(),
			sky.atmosphere_radius()
		};

		FrameGraphPassBuilder params_builder = framegraph.add_pass("Rayleigh sky params pass");

		const auto buffer = params_builder.declare_typed_buffer<uniform::RayleighSky>(1);
		const auto sky_light_buffer = params_builder.declare_typed_buffer<uniform::SkyLight>(1);

		params_builder.add_uniform_input(buffer);
		params_builder.map_update(buffer);

		params_builder.add_storage_output(sky_light_buffer);
		params_builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			TypedMapping<uniform::RayleighSky> mapping = self->resources().mapped_buffer(buffer);
			mapping[0] = sky_data;

			const auto& program = recorder.device()->device_resources()[DeviceResources::SkyLightParamsProgram];
			recorder.dispatch(program, math::Vec3ui(1), {self->descriptor_sets()[0]});
		});

		FrameGraphPassBuilder builder = framegraph.add_pass("Rayleigh sky pass");

		if(!lit.is_valid()) {
			pass.lit = lit = builder.declare_copy(pass.lit);
		}

		builder.add_uniform_input(in_depth);
		builder.add_uniform_input(gbuffer.color);
		builder.add_uniform_input(gbuffer.normal);
		builder.add_uniform_input(framegraph.device()->device_resources().brdf_lut());

		builder.add_uniform_input(buffer);
		builder.add_uniform_input(sky_light_buffer);

		builder.add_color_output(lit);
		builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
				auto render_pass = recorder.bind_framebuffer(self->framebuffer());
				const auto* material = recorder.device()->device_resources()[DeviceResources::RayleighSkyMaterialTemplate];
				render_pass.bind_material(material, {self->descriptor_sets()[0]});
				render_pass.draw(vk::DrawIndirectCommand(6, 1));
			});
	}

	return pass;
}

}
