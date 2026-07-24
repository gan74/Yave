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

namespace yave {

// Must match shaders/lib/ddgi.slang
static constexpr u32 ddgi_grid_size = 32;
static constexpr u32 ddgi_radiance_probe_size = 32;
static constexpr u32 ddgi_irradiance_probe_size = ddgi_radiance_probe_size / 2;
static constexpr u32 ddgi_probes_per_atlas_row = 256;
static constexpr u32 ddgi_probe_count = ddgi_grid_size * ddgi_grid_size * ddgi_grid_size;

static math::Vec2ui ddgi_atlas_size(u32 probe_size) {
    const u32 rows = (ddgi_probe_count + ddgi_probes_per_atlas_row - 1) / ddgi_probes_per_atlas_row;
    return math::Vec2ui(ddgi_probes_per_atlas_row * probe_size, rows * probe_size);
}

static FrameGraphImageId trace_radiance(FrameGraph& framegraph, const GBufferPass& gbuffer, const DDGISettings& settings, FrameGraphImageId& distance) {
    const SceneView& scene_view = gbuffer.scene_pass.scene_view;
    const TLAS& tlas = scene_view.scene()->tlas();

    const SceneVisibility& visibility = *gbuffer.scene_pass.visibility.visible;
    const IBLProbe* ibl_probe = visibility.sky_light ? visibility.sky_light->component.probe().get() : nullptr;

    const math::Vec2ui atlas_size = ddgi_atlas_size(ddgi_radiance_probe_size);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("DDGI trace pass");

    const auto radiance = builder.declare_image(VK_FORMAT_B10G11R11_UFLOAT_PACK32, atlas_size);
    const auto dist = builder.declare_image(VK_FORMAT_R16G16_SFLOAT, atlas_size);

    const struct Params {
        float probe_spacing;
        u32 light_count;
        u32 frame_id;
        float _pad;
    } params {
        settings.probe_spacing,
        u32(visibility.directional_lights.size()),
        u32(framegraph.frame_id()),
        0.0f,
    };

    const auto directional_buffer = builder.declare_typed_buffer<shader::DirectionalLight>(visibility.directional_lights.size());
    builder.map_buffer(directional_buffer);

    builder.add_storage_output(radiance);
    builder.add_storage_output(dist);

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

        recorder.dispatch_threads(device_resources()[DeviceResources::DDGIUpdateProgram], atlas_size, desc_sets);
    });

    distance = dist;
    return radiance;
}

static FrameGraphImageId convolve_irradiance(FrameGraph& framegraph, FrameGraphImageId radiance, u32 sample_count) {
    const math::Vec2ui atlas_size = ddgi_atlas_size(ddgi_irradiance_probe_size);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("DDGI convolve pass");

    const auto irradiance = builder.declare_image(VK_FORMAT_B10G11R11_UFLOAT_PACK32, atlas_size);

    const struct Params {
        u32 sample_count;
    } params {
        std::max(1u, sample_count),
    };

    builder.add_storage_output(irradiance);
    builder.add_uniform_input(radiance, SamplerType::LinearClamp);
    builder.add_inline_input(params);

    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        recorder.dispatch_threads(device_resources()[DeviceResources::DDGIConvolveProgram], atlas_size, self->descriptor_set());
    });

    return irradiance;
}

DDGIPass DDGIPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, const DDGISettings& settings) {
    if(!raytracing_enabled()) {
        return {};
    }

    FrameGraphImageId distance;
    const FrameGraphImageId radiance = trace_radiance(framegraph, gbuffer, settings, distance);
    const FrameGraphImageId irradiance = convolve_irradiance(framegraph, radiance, settings.convolve_sample_count);

    DDGIPass pass;
    pass.radiance = radiance;
    pass.irradiance = irradiance;
    pass.distance = distance;
    pass.probe_spacing = settings.probe_spacing;
    return pass;
}

}
