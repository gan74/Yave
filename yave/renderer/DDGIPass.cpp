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

#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/graphics/device/MaterialAllocator.h>
#include <yave/graphics/images/TextureLibrary.h>
#include <yave/components/SkyLightComponent.h>

#include <yave/graphics/shader_structs.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/device/MeshAllocator.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/assets/AssetPtr.h>

namespace yave {

// Must match shaders/lib/ddgi.slang
static constexpr u32 ddgi_grid_size = 32;
static constexpr u32 ddgi_radiance_probe_size = 32;
static constexpr u32 ddgi_irradiance_probe_size = ddgi_radiance_probe_size / 2;
static constexpr u32 ddgi_probes_per_atlas_row = 256;
static constexpr u32 ddgi_probe_count = ddgi_grid_size * ddgi_grid_size * ddgi_grid_size;

static constexpr ImageUsage ddgi_atlas_usage = ImageUsage::TextureBit | ImageUsage::StorageBit;

static const FrameGraphPersistentResourceId persistent_radiance_id = FrameGraphPersistentResourceId::create();
static const FrameGraphPersistentResourceId persistent_distance_id = FrameGraphPersistentResourceId::create();
static const FrameGraphPersistentResourceId persistent_irradiance_id = FrameGraphPersistentResourceId::create();

static math::Vec2ui ddgi_atlas_size(u32 probe_size) {
    const u32 rows = (ddgi_probe_count + ddgi_probes_per_atlas_row - 1) / ddgi_probes_per_atlas_row;
    return math::Vec2ui(ddgi_probes_per_atlas_row * probe_size, rows * probe_size);
}

static void trace_radiance(FrameGraph& framegraph, const GBufferPass& gbuffer, const DDGISettings& settings, const StorageView& radiance, const StorageView& distance, bool reset) {
    const SceneView& scene_view = gbuffer.scene_pass.scene_view;
    const TLAS& tlas = scene_view.scene()->tlas();

    const SceneVisibility& visibility = *gbuffer.scene_pass.visibility.visible;
    const IBLProbe* ibl_probe = visibility.sky_light ? visibility.sky_light->component.probe().get() : nullptr;

    const math::Vec2ui atlas_size = ddgi_atlas_size(ddgi_radiance_probe_size);
    const u32 probe_update_stride = std::max(1u, settings.probe_update_stride);

    const struct Params {
        float probe_spacing;
        u32 light_count;
        u32 frame_id;
        u32 reset;
        u32 probe_update_stride;
        float blend_alpha;
        float padding;
    } params {
        settings.probe_spacing,
        u32(visibility.directional_lights.size()),
        u32(framegraph.frame_id()),
        u32(reset ? 1 : 0),
        probe_update_stride,
        1.0f / float(probe_update_stride),
        0.0f,
    };

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("DDGI trace pass");

    const auto directional_buffer = builder.declare_typed_buffer<shader::DirectionalLight>(visibility.directional_lights.size());
    builder.map_buffer(directional_buffer);

    builder.add_descriptor_binding(Descriptor(radiance));
    builder.add_descriptor_binding(Descriptor(distance));

    builder.add_descriptor_binding(Descriptor(tlas));

    builder.add_external_input(ibl_probe ? *ibl_probe : *device_resources().empty_probe());
    builder.add_external_input(Descriptor(material_allocator().material_buffer()));
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

        recorder.dispatch_threads(device_resources()[DeviceResources::DDGITraceProgram], atlas_size, desc_sets);
    });
}

static void convolve_irradiance(FrameGraph& framegraph, const DDGISettings& settings, const TextureView& radiance, const StorageView& irradiance, bool reset) {
    const math::Vec2ui atlas_size = ddgi_atlas_size(ddgi_irradiance_probe_size);
    const u32 probe_update_stride = std::max(1u, settings.probe_update_stride);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("DDGI convolve pass");

    const struct Params {
        u32 sample_count;
        u32 frame_id;
        u32 reset;
        u32 probe_update_stride;
    } params {
        std::max(1u, settings.convolve_sample_count),
        u32(framegraph.frame_id()),
        u32(reset ? 1 : 0),
        probe_update_stride,
    };

    builder.add_descriptor_binding(Descriptor(irradiance));
    builder.add_external_input(Descriptor(radiance, SamplerType::LinearClamp));
    builder.add_inline_input(params);

    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        recorder.dispatch_threads(device_resources()[DeviceResources::DDGIConvolveProgram], atlas_size, self->descriptor_set());
    });
}

static FrameGraphImageId apply_gi(FrameGraph& framegraph, const GBufferPass& gbuffer, const TextureView& irradiance, const TextureView& distance, float probe_spacing) {
    const math::Vec2ui size = framegraph.image_size(gbuffer.depth);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("DDGI apply pass");

    const auto gi = builder.declare_image(VK_FORMAT_B10G11R11_UFLOAT_PACK32, size);

    const struct Params {
        float probe_spacing;
    } params {
        probe_spacing,
    };

    builder.add_storage_output(gi);
    builder.add_uniform_input(gbuffer.depth, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.normal, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_external_input(Descriptor(irradiance, SamplerType::LinearClamp));
    builder.add_external_input(Descriptor(distance, SamplerType::LinearClamp));
    builder.add_inline_input(params);

    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        recorder.dispatch_threads(device_resources()[DeviceResources::DDGIApplyProgram], size, self->descriptor_set());
    });

    return gi;
}

DDGIPass DDGIPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, const DDGISettings& settings) {
    if(!raytracing_enabled()) {
        return {};
    }

    const auto regio = framegraph.region("DDGI");

    const math::Vec2ui radiance_atlas_size = ddgi_atlas_size(ddgi_radiance_probe_size);
    const math::Vec2ui irradiance_atlas_size = ddgi_atlas_size(ddgi_irradiance_probe_size);

    const auto [radiance, radiance_reset] = framegraph.create_scratch_image(persistent_radiance_id, VK_FORMAT_B10G11R11_UFLOAT_PACK32, radiance_atlas_size, ddgi_atlas_usage);
    const auto [distance, distance_reset] = framegraph.create_scratch_image(persistent_distance_id, VK_FORMAT_R16G16_SFLOAT, radiance_atlas_size, ddgi_atlas_usage);
    const auto [irradiance, irradiance_reset] = framegraph.create_scratch_image(persistent_irradiance_id, VK_FORMAT_B10G11R11_UFLOAT_PACK32, irradiance_atlas_size, ddgi_atlas_usage);

    const bool reset = radiance_reset || distance_reset || irradiance_reset;
    const TransientImageView<ImageUsage::TextureBit | ImageUsage::StorageBit> radiance_view(radiance);
    const TransientImageView<ImageUsage::TextureBit | ImageUsage::StorageBit> distance_view(distance);
    const TransientImageView<ImageUsage::TextureBit | ImageUsage::StorageBit> irradiance_view(irradiance);

    trace_radiance(framegraph, gbuffer, settings, radiance_view, distance_view, reset);
    convolve_irradiance(framegraph, settings, radiance_view, irradiance_view, reset);

    DDGIPass pass;
    pass.radiance = radiance_view;
    pass.distance = distance_view;
    pass.irradiance = irradiance_view;
    pass.gi = apply_gi(framegraph, gbuffer, irradiance_view, distance_view, settings.probe_spacing);
    pass.probe_spacing = settings.probe_spacing;
    return pass;
}

DDGIProbeDebugPass DDGIProbeDebugPass::create(FrameGraph& framegraph, FrameGraphImageId in_lit, const GBufferPass& gbuffer, const DDGIPass& ddgi, const DDGIProbeDebugSettings& settings) {
    if(settings.debug_mode == DDGIProbeDebugMode::None || !ddgi.gi.is_valid()) {
        return {in_lit, gbuffer.depth};
    }

    const bool display_irradiance = settings.debug_mode == DDGIProbeDebugMode::Irradiance;

    FrameGraphPassBuilder builder = framegraph.add_pass("DDGI probe debug pass");

    const auto color = builder.declare_copy(in_lit);
    const auto depth = builder.declare_copy(gbuffer.depth);

    const float sphere_size = 0.1f;
    const AssetPtr<StaticMesh> sphere = device_resources()[DeviceResources::SimpleSphereMesh];

    const struct Params {
        float probe_spacing;
        float probe_radius;
        u32 mesh_data_index;
        u32 display_irradiance;
    } params {
        ddgi.probe_spacing,
        sphere_size * ddgi.probe_spacing,
        sphere->mesh_data_index(),
        display_irradiance ? 1u : 0u,
    };

    builder.add_color_output(color);
    builder.add_depth_output(depth);

    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_external_input(Descriptor(mesh_allocator().mesh_data_buffer()));
    builder.add_external_input(Descriptor(display_irradiance ? ddgi.irradiance : ddgi.radiance, SamplerType::LinearClamp));
    builder.add_inline_input(params);

    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        const MaterialTemplate* material = device_resources()[DeviceResources::DDGIProbeDebugMaterialTemplate];
        render_pass.bind_material_template(material, self->descriptor_set());
        render_pass.draw(sphere->draw_data(), ddgi_probe_count);
    });

    return {color, depth};
}

}
