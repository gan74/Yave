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

#include "RTGIPass.h"
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

#include <yave/graphics/shader_structs.h>

#include <yave/utils/DebugValues.h>

namespace yave {

RTGIPass RTGIPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId /*in_lit*/, const RTGISettings& settings) {
    if(!raytracing_enabled()) {
        return {};
    }

    const u32 hash_size = u32(1) << std::min(settings.hash_size, 30u);
    const math::Vec2ui size = framegraph.image_size(gbuffer.depth);

    const SceneView& scene_view = gbuffer.scene_pass.scene_view;
    const TLAS& tlas = scene_view.scene()->tlas();

    const SceneVisibility& visibility = *gbuffer.scene_pass.visibility.visible;
    const IBLProbe* ibl_probe = visibility.sky_light ? visibility.sky_light->component.probe().get() : nullptr;


    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("RTGI pass");

    const auto gi = builder.declare_image(VK_FORMAT_B10G11R11_UFLOAT_PACK32, size);

    static const FrameGraphPersistentResourceId persistent_hash_id = FrameGraphPersistentResourceId::create();
    static const FrameGraphPersistentResourceId persistent_sum_id = FrameGraphPersistentResourceId::create();
    const auto [hash, hash_reset] = framegraph.create_scratch_buffer<u32, BufferUsage::StorageBit>(persistent_hash_id, hash_size * 4);
    const auto [sum, sum_reset] = framegraph.create_scratch_buffer<math::Vec4, BufferUsage::StorageBit>(persistent_sum_id, hash_size);

    const bool reset = editor::debug_values().command("Reset RTGI");

    const struct Params {
        u32 hash_size;
        u32 frame_id;
        u32 reset_hash;
        float lod_jitter_strength;

        float lod_dist;
        float base_cell_size;
        float pos_jitter_strength;
        float norm_jitter_strength;

        float max_samples;
        float min_ray_count;
        float max_ray_count;
        u32 light_count;
    } params {
        hash_size,
        u32(framegraph.frame_id()),
        u32(hash_reset || sum_reset || reset ? 1 : 0),
        settings.lod_jitter,

        settings.lod_dist,
        settings.base_cell_size,
        settings.pos_jitter,
        settings.norm_jitter,

        256.0f,
        settings.min_ray_count,
        settings.max_ray_count,
        u32(visibility.directional_lights.size()),
    };

    const auto directional_buffer = builder.declare_typed_buffer<shader::DirectionalLight>(visibility.directional_lights.size());
    builder.map_buffer(directional_buffer);

    builder.add_uniform_input(gbuffer.depth, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.normal, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_descriptor_binding(Descriptor(hash));
    builder.add_descriptor_binding(Descriptor(sum));
    builder.add_inline_input(params);

    builder.add_descriptor_binding(Descriptor(tlas));
    builder.add_external_input(Descriptor(material_allocator().material_buffer()));
    builder.add_storage_input(directional_buffer);
    builder.add_external_input(ibl_probe ? *ibl_probe : *device_resources().empty_probe());

    builder.add_storage_output(gi);

    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        auto mapping = self->resources().map_buffer(directional_buffer);
        for(usize i = 0; i != visibility.directional_lights.size(); ++i) {
            const DirectionalLightComponent& light = visibility.directional_lights[i]->component;
            mapping[i] = {
                -light.direction().normalized(),
                std::cos(light.disk_size()),
                light.color() * light.intensity(),
                u32(light.cast_shadow() ? 1 : 0), {}
            };
        }

        const std::array<DescriptorSetProxy, 2> desc_sets = {
            self->descriptor_set(),
            texture_library().descriptor_set()
        };

        const std::array<BufferBarrier, 2> barriers = {
            BufferBarrier(hash, PipelineStage::ComputeBit, PipelineStage::ComputeBit),
            BufferBarrier(sum, PipelineStage::ComputeBit, PipelineStage::ComputeBit),
        };

        recorder.dispatch_threads(device_resources()[DeviceResources::RTGITrimProgram], math::Vec2ui(hash_size, 1), desc_sets);
        recorder.barriers(barriers);
        recorder.dispatch_threads(device_resources()[DeviceResources::RTGICountProgram], size, desc_sets);
        recorder.barriers(barriers);
        recorder.dispatch_threads(device_resources()[DeviceResources::RTGIUpdateProgram], size, desc_sets);
        recorder.barriers(barriers);
        recorder.dispatch_threads(device_resources()[DeviceResources::RTGIApplyProgram], size, desc_sets);
    });

    return {gi};
}


}

