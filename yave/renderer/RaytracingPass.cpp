/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include "RaytracingPass.h"
#include "GBufferPass.h"
#include "TAAPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>

#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/graphics/device/MaterialAllocator.h>
#include <yave/graphics/images/TextureLibrary.h>
#include <yave/components/SkyLightComponent.h>
#include <yave/components/SkyLightComponent.h>

#include <yave/graphics/shader_structs.h>

#include <y/math/random.h>

namespace yave {

template<usize U>
static auto generate_sample_dirs(u64 seed) {
    y_profile();

    std::array<math::Vec4, U> dirs;
    const float golden = (1.0f + std::sqrt(5.0f)) * 0.5f;
    for(usize i = 0; i != dirs.size(); ++i) {
        const float wrapped_index = float((i + usize(seed)) % dirs.size());

        const float theta = 2.0f * math::pi<float> * wrapped_index * golden;
        const float phi = std::acos(1.0f - 2.0f * wrapped_index / float(dirs.size()));
        dirs[i] = math::Vec4(math::Vec2(std::cos(theta), std::sin(theta)) * std::sin(phi), std::cos(phi), 0.0f);
    }

    std::shuffle(dirs.begin(), dirs.end(), math::FastRandom(seed));

    return dirs;
}


RaytracingPass RaytracingPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, const math::Vec2ui& size) {
    const auto sample_dirs = generate_sample_dirs<256>(framegraph.frame_id());

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Raytracing");

    const SceneView& scene_view = gbuffer.scene_pass.scene_view;
    const SceneVisibility& visibility = *gbuffer.scene_pass.visibility.visible;

    const u32 directional_count = u32(visibility.directional_lights.size());

    const IBLProbe* ibl_probe = visibility.sky_light ? visibility.sky_light->component.probe().get() : nullptr;
    const TLAS& tlas = scene_view.scene()->tlas();

    const auto raytraced = builder.declare_image(VK_FORMAT_R8G8B8A8_UNORM, size);
    const auto sample_dir_buffer = builder.declare_typed_buffer<std::remove_cvref_t<decltype(sample_dirs)>>();
    const auto directional_buffer = builder.declare_typed_buffer<shader::DirectionalLight>(visibility.directional_lights.size());

    builder.map_buffer(sample_dir_buffer, sample_dirs);
    builder.map_buffer(directional_buffer);

    builder.add_descriptor_binding(Descriptor(tlas));
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(gbuffer.normal);
    builder.add_external_input(Descriptor(material_allocator().material_buffer()));
    builder.add_external_input(ibl_probe ? *ibl_probe : *device_resources().empty_probe());
    builder.add_uniform_input(sample_dir_buffer);
    builder.add_storage_input(directional_buffer);
    builder.add_storage_output(raytraced);
    builder.add_inline_input(InlineDescriptor(directional_count));
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        auto mapping = self->resources().map_buffer(directional_buffer);
        for(usize i = 0; i != visibility.directional_lights.size(); ++i) {
            const DirectionalLightComponent& light = visibility.directional_lights[i]->component;
            mapping[i] = {
                -light.direction().normalized(),
                std::cos(light.disk_size()),
                light.color() * light.intensity(),
                0, {}
            };
        }

        const std::array<DescriptorSetProxy, 2> desc_sets = {
            self->descriptor_set(),
            texture_library().descriptor_set()
        };
        recorder.dispatch_threads(device_resources()[DeviceResources::RTProgram], size, desc_sets);
    });

    static const FrameGraphPersistentResourceId persistent_color_id = FrameGraphPersistentResourceId::create();
    static const FrameGraphPersistentResourceId persistent_motion_id = FrameGraphPersistentResourceId::create();

    RaytracingPass pass;
    pass.raytraced =  TAAPass::create(framegraph, gbuffer, raytraced, persistent_color_id, persistent_motion_id).anti_aliased;
    return pass;
}


}

