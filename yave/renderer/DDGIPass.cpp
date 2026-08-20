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

#include "DDGIPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/shaders/ComputeProgram.h>

#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/device/MeshAllocator.h>
#include <yave/graphics/device/MaterialAllocator.h>
#include <yave/graphics/images/TextureLibrary.h>

#include <yave/components/SkyLightComponent.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/assets/AssetPtr.h>

#include <yave/graphics/shader_structs.h>

namespace yave {

// Must match shaders/lib/ddgi.slang
static constexpr u32 ddgi_grid_size = 32;
static constexpr u32 ddgi_radiance_probe_size = 32;
static constexpr u32 ddgi_irradiance_probe_size = ddgi_radiance_probe_size / 2;
static constexpr u32 ddgi_probe_border = 1;
static constexpr u32 ddgi_radiance_probe_data_size = ddgi_radiance_probe_size - 2 * ddgi_probe_border;
static constexpr u32 ddgi_irradiance_probe_data_size = ddgi_irradiance_probe_size - 2 * ddgi_probe_border;
static constexpr u32 ddgi_radiance_probe_border_texel_count = 4 * ddgi_radiance_probe_size - 4;
static constexpr u32 ddgi_irradiance_probe_border_texel_count = 4 * ddgi_irradiance_probe_size - 4;
static constexpr u32 ddgi_probes_per_atlas_row = 256;
static constexpr u32 ddgi_grid_cell_count = ddgi_grid_size * ddgi_grid_size * ddgi_grid_size;
static constexpr u32 ddgi_max_visible_age = 120;

static constexpr ImageUsage ddgi_atlas_usage = ImageUsage::TextureBit | ImageUsage::StorageBit;
static constexpr ImageUsage ddgi_probe_grid_usage = ImageUsage::TextureBit | ImageUsage::StorageBit;

static const FrameGraphPersistentResourceId persistent_radiance_id = FrameGraphPersistentResourceId::create();
static const FrameGraphPersistentResourceId persistent_distance_id = FrameGraphPersistentResourceId::create();
static const FrameGraphPersistentResourceId persistent_irradiance_id = FrameGraphPersistentResourceId::create();
static const FrameGraphPersistentResourceId persistent_probe_grid_id = FrameGraphPersistentResourceId::create();
static const FrameGraphPersistentResourceId persistent_active_probes_id = FrameGraphPersistentResourceId::create();
static const FrameGraphPersistentResourceId persistent_probe_data_id = FrameGraphPersistentResourceId::create();

static u32 ddgi_max_probe_count(const DDGISettings& settings) {
    return std::max(1u, settings.max_probe_count);
}

static math::Vec2ui ddgi_atlas_size(u32 probe_size, u32 max_probe_count) {
    const u32 rows = (max_probe_count + ddgi_probes_per_atlas_row - 1) / ddgi_probes_per_atlas_row;
    return math::Vec2ui(ddgi_probes_per_atlas_row * probe_size, rows * probe_size);
}

static math::Vec2ui ddgi_probe_grid_size() {
    return math::Vec2ui(ddgi_grid_size * ddgi_grid_size, ddgi_grid_size);
}

static void select_probes(FrameGraph& framegraph, const GBufferPass& gbuffer, const DDGISettings& settings, const StorageView& probe_grid, const SubBuffer<BufferUsage::StorageBit>& active_probes, const SubBuffer<BufferUsage::StorageBit>& probe_datas, bool reset) {
    const math::Vec2ui size = framegraph.image_size(gbuffer.depth);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("DDGI select pass");

    const struct Params {
        float probe_spacing;
        u32 max_probe_count;
        u32 frame_id;
        u32 max_visible_age;
    } params {
        settings.probe_spacing,
        ddgi_max_probe_count(settings),
        u32(framegraph.frame_id()),
        ddgi_max_visible_age
    };

    builder.add_external_input(Descriptor(probe_grid));
    builder.add_external_input(Descriptor(active_probes));
    builder.add_external_input(Descriptor(probe_datas));
    builder.add_uniform_input(gbuffer.depth, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.normal, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_inline_input(params);

    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const u32 max_probe_count = ddgi_max_probe_count(settings);
        if(reset) {
            recorder.dispatch_threads(device_resources()[DeviceResources::DDGISelectClearProgram], ddgi_probe_grid_size(), self->descriptor_set());
        } else {
            const BufferBarrier active_probes_barrier(active_probes, PipelineStage::ComputeBit, PipelineStage::ComputeBit);
            recorder.dispatch_threads(device_resources()[DeviceResources::DDGISelectTrimClearProgram], math::Vec2ui(1, 1), self->descriptor_set());
            recorder.barriers(active_probes_barrier);
            recorder.dispatch_threads(device_resources()[DeviceResources::DDGISelectTrimProgram], math::Vec2ui(max_probe_count, 1), self->descriptor_set());
            recorder.barriers(active_probes_barrier);
        }
        recorder.dispatch_threads(device_resources()[DeviceResources::DDGISelectProgram], size, self->descriptor_set());
    });
}

static void trace_radiance(FrameGraph& framegraph, const GBufferPass& gbuffer, const DDGISettings& settings, const TextureView& probe_grid, const SubBuffer<BufferUsage::StorageBit>& probe_datas, const StorageView& radiance, const StorageView& distance) {
    const SceneView& scene_view = gbuffer.scene_pass.scene_view;
    const TLAS& tlas = scene_view.scene()->tlas();

    const SceneVisibility& visibility = *gbuffer.scene_pass.visibility.visible;
    const IBLProbe* ibl_probe = visibility.sky_light ? visibility.sky_light->component.probe().get() : nullptr;

    const math::Vec3ui dispatch_size(ddgi_grid_cell_count, ddgi_radiance_probe_data_size, ddgi_radiance_probe_data_size);
    const math::Vec3ui border_dispatch_size(ddgi_grid_cell_count, ddgi_radiance_probe_border_texel_count, 1);

    const struct Params {
        float probe_spacing;
        u32 light_count;
        u32 frame_id;
        u32 padding_0;
    } params {
        settings.probe_spacing,
        u32(visibility.directional_lights.size()),
        u32(framegraph.frame_id()),
        0u
    };

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("DDGI trace pass");

    const auto directional_buffer = builder.declare_typed_buffer<shader::DirectionalLight>(visibility.directional_lights.size());
    builder.map_buffer(directional_buffer);

    builder.add_external_input(Descriptor(radiance));
    builder.add_external_input(Descriptor(distance));

    builder.add_external_input(Descriptor(tlas));

    builder.add_external_input(ibl_probe ? *ibl_probe : *device_resources().empty_probe());
    builder.add_external_input(Descriptor(material_allocator().material_buffer()));
    builder.add_storage_input(directional_buffer);
    builder.add_external_input(Descriptor(probe_grid, SamplerType::PointClamp));
    builder.add_external_input(Descriptor(probe_datas));
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

        const auto& program = device_resources()[DeviceResources::DDGITraceProgram];
        const auto& border_program = device_resources()[DeviceResources::DDGITraceBorderProgram];
        recorder.dispatch_threads(program, dispatch_size, desc_sets);
        recorder.dispatch_threads(border_program, border_dispatch_size, desc_sets);
    });
}

static void convolve_irradiance(FrameGraph& framegraph, const DDGISettings& settings, const TextureView& probe_grid, const TextureView& radiance, const StorageView& irradiance) {
    const math::Vec3ui dispatch_size(ddgi_grid_cell_count, ddgi_irradiance_probe_data_size, ddgi_irradiance_probe_data_size);
    const math::Vec3ui border_dispatch_size(ddgi_grid_cell_count, ddgi_irradiance_probe_border_texel_count, 1);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("DDGI convolve pass");

    const struct Params {
        u32 sample_count;
        u32 atlas_probe_count;
        u32 padding_0;
        u32 padding_1;
    } params {
        std::max(1u, settings.convolve_sample_count),
        ddgi_max_probe_count(settings),
        0u, 0u
    };

    builder.add_external_input(Descriptor(irradiance));
    builder.add_external_input(Descriptor(radiance, SamplerType::LinearClamp));
    builder.add_external_input(Descriptor(probe_grid, SamplerType::PointClamp));
    builder.add_inline_input(params);

    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const auto& program = device_resources()[DeviceResources::DDGIConvolveProgram];
        const auto& border_program = device_resources()[DeviceResources::DDGIConvolveBorderProgram];
        recorder.dispatch_threads(program, dispatch_size, self->descriptor_set());
        recorder.dispatch_threads(border_program, border_dispatch_size, self->descriptor_set());
    });
}

static FrameGraphImageId apply_gi(FrameGraph& framegraph, const GBufferPass& gbuffer, const TextureView& probe_grid, const TextureView& irradiance, const TextureView& distance, const DDGISettings& settings) {
    const math::Vec2ui size = framegraph.image_size(gbuffer.depth);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("DDGI apply pass");

    const auto gi = builder.declare_image(VK_FORMAT_B10G11R11_UFLOAT_PACK32, size);

    const struct Params {
        float probe_spacing;
        u32 atlas_probe_count;
        u32 padding_0;
        u32 padding_1;
    } params {
        settings.probe_spacing,
        ddgi_max_probe_count(settings),
        0u, 0u
    };

    builder.add_storage_output(gi);
    builder.add_uniform_input(gbuffer.depth, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.normal, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.scene_pass.camera);

    Y_TODO(probe filtering)
    builder.add_external_input(Descriptor(irradiance, SamplerType::LinearClamp));
    builder.add_external_input(Descriptor(distance, SamplerType::LinearClamp));
    builder.add_external_input(Descriptor(probe_grid, SamplerType::PointClamp));
    builder.add_inline_input(params);

    make_simple_compute_pass(builder, DeviceResources::DDGIApplyProgram, size);

    return gi;
}

DDGIPass DDGIPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, const DDGISettings& settings) {
    if(!raytracing_enabled()) {
        return {};
    }

    const auto region = framegraph.region("DDGI");

    const u32 max_probe_count = ddgi_max_probe_count(settings);
    const math::Vec2ui radiance_atlas_size = ddgi_atlas_size(ddgi_radiance_probe_size, max_probe_count);
    const math::Vec2ui irradiance_atlas_size = ddgi_atlas_size(ddgi_irradiance_probe_size, max_probe_count);
    const math::Vec2ui probe_grid_size = ddgi_probe_grid_size();

    const auto& [radiance,      radiance_reset]         = framegraph.create_scratch_image(persistent_radiance_id, VK_FORMAT_B10G11R11_UFLOAT_PACK32, radiance_atlas_size, ddgi_atlas_usage);
    const auto& [distance,      distance_reset]         = framegraph.create_scratch_image(persistent_distance_id, VK_FORMAT_R16G16_UNORM, radiance_atlas_size, ddgi_atlas_usage);
    const auto& [irradiance,    irradiance_reset]       = framegraph.create_scratch_image(persistent_irradiance_id, VK_FORMAT_B10G11R11_UFLOAT_PACK32, irradiance_atlas_size, ddgi_atlas_usage);
    const auto& [probe_grid ,   probe_grid_reset]       = framegraph.create_scratch_image(persistent_probe_grid_id, VK_FORMAT_R32_UINT, probe_grid_size, ddgi_probe_grid_usage);
    const auto& [active_probes, active_probes_reset]    = framegraph.create_scratch_buffer<u32, BufferUsage::StorageBit>(persistent_active_probes_id, max_probe_count + 2);
    const auto& [probe_datas,   probe_datas_reset]       = framegraph.create_scratch_buffer<shader::DDGIProbeData, BufferUsage::StorageBit>(persistent_probe_data_id, max_probe_count);

    const bool reset = radiance_reset || distance_reset || irradiance_reset || probe_grid_reset || active_probes_reset || probe_datas_reset;

    const TransientImageView<ddgi_atlas_usage> radiance_view(radiance);
    const TransientImageView<ddgi_atlas_usage> distance_view(distance);
    const TransientImageView<ddgi_atlas_usage> irradiance_view(irradiance);
    const TransientImageView<ddgi_probe_grid_usage> probe_grid_view(probe_grid);

    select_probes(framegraph, gbuffer, settings, probe_grid_view, active_probes, probe_datas, reset);
    trace_radiance(framegraph, gbuffer, settings, probe_grid_view, probe_datas, radiance_view, distance_view);
    convolve_irradiance(framegraph, settings, probe_grid_view, radiance_view, irradiance_view);

    DDGIPass pass;
    pass.radiance = radiance_view;
    pass.distance = distance_view;
    pass.irradiance = irradiance_view;
    pass.probe_grid = probe_grid_view;
    pass.active_probes = active_probes;
    pass.probe_datas = probe_datas;
    pass.gi = apply_gi(framegraph, gbuffer, probe_grid_view, irradiance_view, distance_view, settings);
    pass.probe_spacing = settings.probe_spacing;
    pass.max_probe_count = max_probe_count;
    return pass;
}

DDGIProbeDebugPass DDGIProbeDebugPass::create(FrameGraph& framegraph, FrameGraphImageId in_lit, const GBufferPass& gbuffer, const DDGIPass& ddgi, const DDGIProbeDebugSettings& settings) {
    if(settings.debug_mode == DDGIProbeDebugMode::None || !ddgi.gi.is_valid()) {
        return {in_lit, gbuffer.depth};
    }

    FrameGraphMutableImageId color;
    FrameGraphMutableImageId depth;

    {
        FrameGraphPassBuilder builder = framegraph.add_pass("DDGI probe debug pass");

        color = builder.declare_copy(in_lit);
        depth = builder.declare_copy(gbuffer.depth);

        const float sphere_size = 0.1f;
        const AssetPtr<StaticMesh> sphere = device_resources()[DeviceResources::SimpleSphereMesh];

        const struct Params {
            float probe_spacing;
            u32 atlas_probe_count;
            u32 display_mode;
            float probe_radius;

            u32 mesh_data_index;
            u32 frame_id;
            u32 max_visible_age;
            u32 padding_0;
        } params {
            ddgi.probe_spacing,
            ddgi.max_probe_count,
            u32(settings.debug_mode),
            sphere_size * ddgi.probe_spacing,

            sphere->mesh_data_index(),
            u32(framegraph.frame_id()),
            ddgi_max_visible_age,
            0u
        };

        builder.add_color_output(color);
        builder.add_depth_output(depth);
        builder.add_uniform_input(gbuffer.scene_pass.camera);
        builder.add_external_input(Descriptor(mesh_allocator().mesh_data_buffer()));

        Y_TODO(probe filtering)
        builder.add_external_input(Descriptor(ddgi.radiance, SamplerType::LinearClamp));
        builder.add_external_input(Descriptor(ddgi.irradiance, SamplerType::LinearClamp));
        builder.add_external_input(Descriptor(ddgi.distance, SamplerType::LinearClamp));
        builder.add_external_input(Descriptor(ddgi.probe_grid, SamplerType::PointClamp));
        builder.add_external_input(Descriptor(ddgi.probe_datas));

        builder.add_inline_input(params);

        builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
            const MaterialTemplate* material = device_resources()[DeviceResources::DDGIProbeDebugMaterialTemplate];
            render_pass.bind_material_template(material, self->descriptor_set());
            render_pass.draw(sphere->draw_data(), ddgi_grid_cell_count);
        });
    }

    {
        FrameGraphPassBuilder builder = framegraph.add_pass("DDGI probe count debug pass");

        const auto out_color = builder.declare_copy(color);

        const struct CountParams {
            u32 max_probe_count;
            u32 padding_0;
            u32 padding_1;
            u32 padding_2;
        } count_params {
            ddgi.max_probe_count,
            0u, 0u, 0u
        };

        builder.add_color_output(out_color);
        builder.add_external_input(Descriptor(ddgi.active_probes));
        builder.add_inline_input(count_params);

        make_simple_full_screen_pass(builder, DeviceResources::DDGIProbeCountDebugMaterialTemplate);

        color = out_color;
    }

    return {color, depth};
}

}
