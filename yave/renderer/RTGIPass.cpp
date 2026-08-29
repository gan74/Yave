/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

namespace yave {

RTGIPass RTGIPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId /*in_lit*/, const RTGISettings& settings) {
    if(!raytracing_enabled()) {
        return {};
    }

    const u32 max_updates = u32(1) << 18;
    const u32 hash_size = u32(1) << std::min(settings.hash_size, 30u);
    const math::Vec2ui size = framegraph.image_size(gbuffer.depth);

    const SceneView& scene_view = gbuffer.scene_pass.scene_view;
    const TLAS& tlas = scene_view.scene()->tlas();

    const SceneVisibility& visibility = *gbuffer.scene_pass.visibility.visible;
    const IBLProbe* ibl_probe = visibility.sky_light ? visibility.sky_light->component.probe().get() : nullptr;
    const u32 gi_light_count = u32(std::count_if(visibility.directional_lights.begin(), visibility.directional_lights.end(), [](const auto& l) { return l->component.cast_gi(); }));


    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("RTGI pass");

    const auto gi = builder.declare_image(VK_FORMAT_B10G11R11_UFLOAT_PACK32, size);

    static const FrameGraphPersistentResourceId persistent_hash_id = FrameGraphPersistentResourceId::create();
    static const FrameGraphPersistentResourceId persistent_sum_id = FrameGraphPersistentResourceId::create();
    static const FrameGraphPersistentResourceId persistent_updates_id = FrameGraphPersistentResourceId::create();
    static const FrameGraphPersistentResourceId persistent_update_count_id = FrameGraphPersistentResourceId::create();
    const auto [hash, hash_reset] = framegraph.create_scratch_buffer<u32, BufferUsage::StorageBit>(persistent_hash_id, hash_size * 2);
    const auto [sum, sum_reset] = framegraph.create_scratch_buffer<shader::RTGICell, BufferUsage::StorageBit>(persistent_sum_id, hash_size);
    const auto [updates, updates_reset] = framegraph.create_scratch_buffer<shader::RTGIUpdate, BufferUsage::StorageBit>(persistent_updates_id, max_updates);
    const auto [update_count, update_count_reset] = framegraph.create_scratch_buffer<u32, BufferUsage::StorageBit>(persistent_update_count_id);

    const bool reset = false; // editor::debug_values().command("Reset RTGI");

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

        u32 max_updates;
        u32 padding_0;
        u32 padding_1;
        u32 padding_2;
    } params {
        hash_size,
        u32(framegraph.frame_id()),
        u32(hash_reset || sum_reset || updates_reset || update_count_reset || reset ? 1 : 0),
        settings.lod_jitter,

        settings.lod_dist,
        settings.base_cell_size,
        settings.pos_jitter,
        settings.norm_jitter,

        4096.0f,
        settings.min_ray_count,
        settings.max_ray_count,
        gi_light_count,

        max_updates,
        0u, 0u, 0u
    };

    const auto directional_buffer = builder.declare_typed_buffer<shader::DirectionalLight>(visibility.directional_lights.size());
    builder.map_buffer(directional_buffer);

    builder.add_storage_output(gi);

    builder.add_descriptor_binding(Descriptor(hash));
    builder.add_descriptor_binding(Descriptor(sum));
    builder.add_descriptor_binding(Descriptor(tlas));
    builder.add_uniform_input(gbuffer.depth, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.normal, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_external_input(ibl_probe ? *ibl_probe : *device_resources().empty_probe());
    builder.add_external_input(Descriptor(material_allocator().material_buffer()));
    builder.add_storage_input(directional_buffer);
    builder.add_descriptor_binding(Descriptor(updates));
    builder.add_descriptor_binding(Descriptor(update_count));
    builder.add_inline_input(params);

    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        {
            u32 index = 0;
            auto mapping = self->resources().map_buffer(directional_buffer);
            for(usize i = 0; i != visibility.directional_lights.size(); ++i) {
                const DirectionalLightComponent& light = visibility.directional_lights[i]->component;
                if(!light.cast_gi()) {
                    continue;
                }
                mapping[index++] = {
                    -light.direction().normalized(),
                    std::cos(light.disk_size()),
                    light.color() * light.intensity(),
                    u32(light.cast_shadow() ? 1 : 0), {}
                };
            }
            y_debug_assert(index == gi_light_count);
        }

        const std::array<DescriptorSetProxy, 2> desc_sets = {
            self->descriptor_set(),
            texture_library().descriptor_set()
        };

        const std::array<BufferBarrier, 4> barriers = {
            BufferBarrier(hash, PipelineStage::ComputeBit, PipelineStage::ComputeBit),
            BufferBarrier(sum, PipelineStage::ComputeBit, PipelineStage::ComputeBit),
            BufferBarrier(updates, PipelineStage::ComputeBit, PipelineStage::ComputeBit),
            BufferBarrier(update_count, PipelineStage::ComputeBit, PipelineStage::ComputeBit),
        };

        recorder.dispatch_threads(device_resources()[DeviceResources::RTGITrimProgram], math::Vec2ui(hash_size, 1), desc_sets);
        recorder.barriers(barriers);
        recorder.dispatch_threads(device_resources()[DeviceResources::RTGIUpdateProgram], size, desc_sets);
        recorder.barriers(barriers);
        recorder.dispatch_threads(device_resources()[DeviceResources::RTGITraceProgram], math::Vec2ui(hash_size, 1), desc_sets);
        recorder.barriers(barriers);
        recorder.dispatch_threads(device_resources()[DeviceResources::RTGIApplyProgram], size, desc_sets);
    });

    return {gi};
}


}
