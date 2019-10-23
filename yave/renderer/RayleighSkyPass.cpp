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

#include <yave/components/DirectionalLightComponent.h>
#include <y/core/Chrono.h>


namespace yave {

static DirectionalLightComponent sun_data(const ecs::EntityWorld& world) {
	DirectionalLightComponent sun;
	sun.intensity() = 0.0f;
	for(auto [l] : world.view<DirectionalLightComponent>().components()) {
		if(sun.intensity() < l.intensity()) {
			sun = l;
		}
	}
	return sun;

	/*float time = float(core::Chrono::program().to_secs());
	return math::Vec4(std::cos(time), 0.0f, std::sin(time), 20.0f);*/
}

RayleighSkyPass RayleighSkyPass::create(FrameGraph& framegraph, const SceneView& scene_view, FrameGraphImageId in_depth, FrameGraphImageId in_color) {

	DirectionalLightComponent sun = sun_data(scene_view.world());
	struct SkyData {
		uniform::LightingCamera camera_data;
		math::Vec3 sun_direction;
		float origin_height;

		math::Vec3 sun_color;
		float planet_height;
		float atmo_height;
	} sky_data {
			scene_view.camera(),
			-sun.direction().normalized(),
			6360.0f * 1000.0f + 100.0f /*+ std::pow(float(core::Chrono::program().to_secs()) * 3.0f, 3.0f)*/,
			sun.color() * sun.intensity(),
			6360.0f * 1000.0f,
			6420.0f * 1000.0f,
		};


	FrameGraphPassBuilder builder = framegraph.add_pass("Rayleigh sky pass");

	auto depth = builder.declare_copy(in_depth);
	auto color = builder.declare_copy(in_color);
	auto buffer = builder.declare_typed_buffer<SkyData>(1);

	RayleighSkyPass pass;
	pass.depth = depth;
	pass.color = color;

	builder.add_uniform_input(buffer);
	builder.map_update(buffer);

	builder.add_depth_output(depth, Framebuffer::LoadOp::Load);
	builder.add_color_output(color, Framebuffer::LoadOp::Load);
	builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			TypedMapping<SkyData> mapping = self->resources()->mapped_buffer(buffer);
			mapping[0] = sky_data;

			auto render_pass = recorder.bind_framebuffer(self->framebuffer());
			const auto* material = recorder.device()->device_resources()[DeviceResources::RayleighSkyMaterialTemplate];
			render_pass.bind_material(material, {self->descriptor_sets()[0]});
			render_pass.draw(vk::DrawIndirectCommand(6, 1));
		});

	return pass;
}

}
