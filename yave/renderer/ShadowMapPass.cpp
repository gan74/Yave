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

#include "ShadowMapPass.h"

#include <yave/framegraph/FrameGraph.h>

#include <yave/components/SpotLightComponent.h>
#include <yave/entities/entities.h>
#include <yave/ecs/EntityWorld.h>

namespace yave {

static Camera spotlight_camera(const TransformableComponent& tr, const SpotLightComponent& sp) {
	Camera cam;
	cam.set_proj(math::perspective(sp.half_angle() * 2.0f, 1.0f, 0.1f));
	cam.set_view(math::look_at(tr.position(), tr.position() + tr.forward(), tr.up()));
	return cam;
}

ShadowMapPass ShadowMapPass::create(FrameGraph& framegraph, const SceneView& scene, const ShadowMapPassSettings& settings) {
	const ecs::EntityWorld& world = scene.world();

	FrameGraphPassBuilder builder = framegraph.add_pass("Shadow pass");

	const math::Vec2ui shadow_map_size = settings.shadow_map_size;
	const auto shadow_map = builder.declare_image(vk::Format::eD32Sfloat, shadow_map_size);

	ShadowMapPass pass;
	pass.shadow_map = shadow_map;
	pass.sub_passes = std::make_shared<SubPassData>();

	{
		u32 y = 0;
		const u32 size = shadow_map_size.x();
		const float uv_mul_y = 1.0f / float(shadow_map_size.y());

		for(auto spot : world.view(SpotLightArchetype())) {
			auto [t, l] = spot.components();
			if(!l.cast_shadow()) {
				continue;
			}

			if(y >= shadow_map_size.y()) {
				log_msg("Shadow atlas is too small.", Log::Warning);
				break;
			}

			const SceneView spot_view(&world, spotlight_camera(t, l));
			pass.sub_passes->passes.push_back(SubPass{
				SceneRenderSubPass::create(builder, spot_view)
			});
			pass.sub_passes->lights[spot.index()] = {
				spot_view.camera().viewproj_matrix(),
				math::Vec2(0.0f, y * uv_mul_y),
				math::Vec2(1.0f, size * uv_mul_y)
			};

			y += size;
		}
	}

	builder.add_depth_output(shadow_map);
	builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
		auto render_pass = recorder.bind_framebuffer(self->framebuffer());

		usize index = 0;
		const u32 size = shadow_map_size.x();
		for(const auto& sub_pass : pass.sub_passes->passes) {
			render_pass.set_viewport(Viewport(math::Vec2(size, size), math::Vec2(0, index++ * size)));
			sub_pass.scene_pass.render(render_pass, self);
		}
	});

	return pass;
}

}
