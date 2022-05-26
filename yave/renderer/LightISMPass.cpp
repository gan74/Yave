/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#include "LightISMPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>


#include <yave/components/DirectionalLightComponent.h>
#include <yave/ecs/EntityWorld.h>

namespace yave {

static Camera rsm_directional_camera(const Camera& cam, const DirectionalLightComponent& light) {
    const float size = light.cascade_distance() * 0.5f;
    const math::Vec3 center = cam.position();
    const math::Vec3 cam_fwd = cam.forward() * size;
    const math::Vec3 cam_right = cam.right() * size;
    const math::Vec3 cam_up = cam.up() * size;

    std::array<math::Vec3, 8> corners;
    for(usize i = 0; i != 8; ++i) {
        corners[i] = center +
            cam_fwd   * (i & 0x04 ? -1.0f : 1.0f) +
            cam_right * (i & 0x02 ? -1.0f : 1.0f) +
            cam_up    * (i & 0x01 ? -1.0f : 1.0f);
    }

    const math::Vec3 up = light.direction().cross(cam_fwd);
    const math::Matrix4<> view = math::look_at(center, center - light.direction(), up);

    math::Vec3 max(-std::numeric_limits<float>::max());
    math::Vec3 min(std::numeric_limits<float>::max());
    for(const math::Vec3 c : corners) {
        const math::Vec4 light_space = view * math::Vec4(c, 1.0f);
        for(usize i = 0; i != 3; ++i) {
            max[i] = std::max(max[i], light_space[i]);
            min[i] = std::min(min[i], light_space[i]);
        }
    }

    const float z_factor = 1000.0f;
    const float inv_z_factor = 1.0f / z_factor;
    max.z() *= max.z() < 0.0f ? inv_z_factor : z_factor;
    min.z() *= min.z() > 0.0f ? inv_z_factor : z_factor;

    Camera camera;
    camera.set_view(view);
    camera.set_proj(math::ortho(min.x(), max.x(), min.y(), max.y(), min.z(), max.z()));
    return camera;
}

static const DirectionalLightComponent* find_directional(const SceneView& scene) {
    for(auto light : scene.world().query<DirectionalLightComponent>()) {
        const auto& [l] = light.components();
        if(!l.cast_shadow()) {
            continue;
        }

        return &l;
    }

    return nullptr;
}

LightISMPass LightISMPass::create(FrameGraph& framegraph, const SceneView& scene, const ProbeGenerationPass& probes, const ISMTestPass& ism) {
    const DirectionalLightComponent* directional = find_directional(scene);
    if(!directional) {
        return {};
    }

    const math::Vec2ui rsm_size(256);
    const math::Vec2ui ism_altlas_size = ism.ism_size * ism.ism_count;

    const DirectionalLightComponent light = *directional;
    const SceneView rsm_view(&scene.world(), rsm_directional_camera(scene.camera(), light));

    const auto gbuffer = GBufferPass::create(framegraph, rsm_view, rsm_size);

    FrameGraphPassBuilder builder = framegraph.add_pass("Light ISM pass");

    const auto lit_ism = builder.declare_image(VK_FORMAT_R16G16B16A16_SFLOAT, ism_altlas_size);

    builder.add_storage_output(lit_ism);
    builder.add_uniform_input(ism.ism_atlas);
    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(gbuffer.color);
    builder.add_uniform_input(gbuffer.normal);
    builder.add_uniform_input(gbuffer.scene_pass.camera_buffer);
    builder.add_storage_input(probes.probe_buffer);
    builder.add_inline_input(InlineDescriptor(math::Vec4ui(ism.ism_size, ism.ism_count)));
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const auto& program = device_resources()[DeviceResources::LightISMProgram];
        recorder.dispatch_size(program, ism_altlas_size, {self->descriptor_sets()[0]});
    });


    LightISMPass pass;
    pass.lit_ism = lit_ism;
    return pass;
}

}

