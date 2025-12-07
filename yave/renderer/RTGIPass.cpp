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
#include <yave/components/SkyLightComponent.h>

#include <yave/graphics/shader_structs.h>

#include <yave/utils/DebugValues.h>

#include <y/math/random.h>

namespace yave {

static FrameGraphImageId rt_reuse(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId in_gi, const RTGISettings& settings) {
    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("RTGI reuse");

    const math::Vec2ui size = framegraph.image_size(gbuffer.depth);
    const ImageFormat format = framegraph.image_format(in_gi);

    const u32 sample_count = settings.denoise_sample_count;
    const u32 seed = u32(hash_u64(framegraph.frame_id()));

    const auto re = builder.declare_image(format, size);

    builder.clear_before_pass(re);

    builder.add_storage_output(re);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_uniform_input(gbuffer.depth, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.normal, SamplerType::PointClamp);
    builder.add_uniform_input(in_gi, SamplerType::PointClamp);
    builder.add_inline_input(math::Vec2ui(seed, sample_count));
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        recorder.dispatch_threads(device_resources()[DeviceResources::RTReuseProgram], size, self->descriptor_set());
    });

    return re;
}


RTGIPass RTGIPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId in_lit, const RTGISettings& settings) {
    if(!raytracing_enabled()) {
        return {in_lit};
    }

    const u32 hash_size = 4 * 1024 * 1024;
    const math::Vec2ui size = framegraph.image_size(gbuffer.depth);

    const auto region = framegraph.region("RTGI");

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("RTGI gather pass");

    const auto gi = builder.declare_image(VK_FORMAT_R16G16B16A16_SFLOAT, size);

    static const FrameGraphPersistentResourceId persistent_hash_id = FrameGraphPersistentResourceId::create();
    static const FrameGraphPersistentResourceId persistent_sum_id = FrameGraphPersistentResourceId::create();
    const auto [hash, hash_reset] = framegraph.create_scratch_buffer<u32, BufferUsage::StorageBit>(persistent_hash_id, hash_size * 2);
    const auto [sum, sum_reset] = framegraph.create_scratch_buffer<math::Vec4, BufferUsage::StorageBit>(persistent_sum_id, hash_size);

    const struct Params {
        u32 hash_size;
        float lod_dist;
        float base_cell_size;
        u32 frame_id;

        u32 reset_hash;
        float max_samples;
        u32 padding1;
        u32 padding2;
    } params {
        hash_size,
        10.0f,
        0.05f,
        u32(framegraph.frame_id()),

        (hash_reset || sum_reset) ? 1 : 0,
        float(16 * 1024),
        0,
        0
    };


    const SceneView& scene_view = gbuffer.scene_pass.scene_view;
    const TLAS& tlas = scene_view.scene()->tlas();

    const SceneVisibility& visibility = *gbuffer.scene_pass.visibility.visible;
    const IBLProbe* ibl_probe = visibility.sky_light ? visibility.sky_light->component.probe().get() : nullptr;

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

        recorder.dispatch_threads(device_resources()[DeviceResources::RTGI2TrimProgram], math::Vec2ui(hash_size, 1), desc_sets);
        recorder.barriers(barriers);
        recorder.dispatch_threads(device_resources()[DeviceResources::RTGI2Program], size, desc_sets);
        recorder.barriers(barriers);
        recorder.dispatch_threads(device_resources()[DeviceResources::RTGI2ApplyProgram], size, desc_sets);
    });


    return {gi};




#if 0
    const auto region = framegraph.region("RTGI");

    const auto sample_dirs = generate_sample_dirs<256>(framegraph.frame_id());

    const u32 resolution_factor = (1 << settings.resolution_scale);
    const math::Vec2ui size = framegraph.image_size(in_lit);
    const math::Vec2ui scaled_size = size / resolution_factor;

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("RTGI pass");

    const SceneView& scene_view = gbuffer.scene_pass.scene_view;
    const SceneVisibility& visibility = *gbuffer.scene_pass.visibility.visible;

    const IBLProbe* ibl_probe = visibility.sky_light ? visibility.sky_light->component.probe().get() : nullptr;
    const TLAS& tlas = scene_view.scene()->tlas();

    const auto gi = builder.declare_image(VK_FORMAT_R16G16B16A16_SFLOAT, scaled_size);

    const auto directional_buffer = builder.declare_typed_buffer<shader::DirectionalLight>(visibility.directional_lights.size());

    // const u32 directional_count = u32(visibility.directional_lights.size());
    struct Params {
        u32 sample_count;
        u32 resolution_scale;
        u32 frame_id;
        u32 padding;
    } params {
        settings.sample_count,
        settings.resolution_scale,
        u32(framegraph.frame_id()),
        0
    };

    builder.map_buffer(directional_buffer);

    builder.add_storage_output(gi);
    builder.add_descriptor_binding(Descriptor(tlas));
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_uniform_input(gbuffer.depth, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.normal, SamplerType::PointClamp);
    builder.add_uniform_input(in_lit, SamplerType::PointClamp);
    builder.add_external_input(Descriptor(material_allocator().material_buffer()));
    builder.add_external_input(ibl_probe ? *ibl_probe : *device_resources().empty_probe());
    builder.add_storage_input(directional_buffer);
    builder.add_inline_input(params);
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
        recorder.dispatch_threads(device_resources()[DeviceResources::RTGIProgram], scaled_size, desc_sets);
    });


    RTGIPass pass;
    pass.gi = gi;

    {
        const auto filter_region = framegraph.region("Filtering");

        for(u32 i = 0; i < settings.denoise_iterations; ++i) {
            pass.gi = rt_reuse(framegraph, gbuffer, pass.gi, settings);
        }

        if(settings.temporal) {
            static const FrameGraphPersistentResourceId persistent_color_id = FrameGraphPersistentResourceId::create();
            static const FrameGraphPersistentResourceId persistent_motion_id = FrameGraphPersistentResourceId::create();

            TAASettings taa_settings;
            {
                taa_settings.use_clamping = editor::debug_values().value<bool>("Use clamping", false);
                taa_settings.use_denoise = editor::debug_values().value<bool>("Use denoise", false);
                taa_settings.luminance_weighting = editor::debug_values().value<bool>("Use lum weighting", false);
                taa_settings.clamping_range = editor::debug_values().value<float>("Clamping range", 1.0f);
                taa_settings.anti_flicker_strength = editor::debug_values().value<float>("Flicker strength", 1.0f);
            }
            pass.gi = TAAPass::create(framegraph, gbuffer, pass.gi, persistent_color_id, persistent_motion_id, taa_settings).anti_aliased;
        }
    }

    return pass;
#endif
}


}

