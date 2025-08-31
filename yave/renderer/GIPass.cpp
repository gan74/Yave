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

#include "GIPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/graphics/device/MaterialAllocator.h>
#include <yave/graphics/images/TextureLibrary.h>


namespace yave {

static constexpr u32 gi_grid_size = 32;
static constexpr u32 gi_probe_count = gi_grid_size * gi_grid_size * gi_grid_size;
static constexpr u32 gi_atlas_size = 256;
static constexpr u32 gi_probe_size = 16;

static_assert(gi_atlas_size * gi_atlas_size >= gi_probe_count);


static const IBLProbe* find_ibl_probe(const SceneVisibility& visibility) {
    if(const SkyLightObject* obj = visibility.sky_light) {
        const SkyLightComponent& sky = obj->component;
        if(const IBLProbe* probe = sky.probe().get()) {
            y_debug_assert(!probe->is_null());
            return probe;
        }
    }

    return device_resources().empty_probe().get();
}


[[maybe_unused]]
static FrameGraphImageId debug_probes(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId lit, FrameGraphImageId probes) {
    FrameGraphPassBuilder builder = framegraph.add_pass("Probe debug pass");

    const auto depth = builder.declare_copy(gbuffer.depth);
    const auto color = builder.declare_copy(lit);

    builder.add_depth_output(depth);
    builder.add_color_output(color);
    builder.add_uniform_input(probes);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        const auto* material = device_resources()[DeviceResources::DebugProbesTemplate];
        render_pass.bind_material_template(material, self->descriptor_set());

        const StaticMesh& sphere = *device_resources()[DeviceResources::SimpleSphereMesh];
        render_pass.draw(sphere.draw_data(), u32(gi_probe_count));
    });

    return color;
}

static FrameGraphImageId trace_probes(FrameGraph& framegraph, const GBufferPass& gbuffer) {
    const SceneView& scene_view = gbuffer.scene_pass.scene_view;
    const TLAS& tlas = scene_view.scene()->tlas();

    const SceneVisibility& visibility = *gbuffer.scene_pass.visibility.visible;
    const IBLProbe* ibl_probe = find_ibl_probe(visibility);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Probe filling pass");

    const auto probes = builder.declare_image(VK_FORMAT_R16G16B16A16_SFLOAT, math::Vec2ui(gi_probe_size * gi_atlas_size));
    const auto directionals = builder.declare_typed_buffer<shader::DirectionalLight>(visibility.directional_lights.size());

    builder.map_buffer(directionals);

    builder.add_external_input(Descriptor(tlas));
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_external_input(Descriptor(material_allocator().material_buffer()));
    builder.add_storage_input(directionals);
    builder.add_external_input(*ibl_probe);
    builder.add_storage_output(probes);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        auto mapping = self->resources().map_buffer(directionals);
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
        const auto& program = device_resources()[DeviceResources::TraceProbesProgram];
        recorder.dispatch_threads(program, math::Vec3ui(gi_probe_size, gi_probe_size, gi_probe_count), desc_sets);
    });

    return probes;
}

static FrameGraphImageId fetch_probes(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId probes) {
    const math::Vec2 size = framegraph.image_size(gbuffer.depth);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Fetch probes pass");

    const auto gi = builder.declare_image(VK_FORMAT_R16G16B16A16_SFLOAT, size);

    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(gbuffer.normal);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_uniform_input(probes);
    builder.add_storage_output(gi);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const auto& program = device_resources()[DeviceResources::FetchProbesProgram];
        recorder.dispatch_threads(program, size, self->descriptor_set());
    });

    return gi;
}


GIPass GIPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId lit) {
    if(!raytracing_enabled()) {
        return {lit};
    }

    const auto region = framegraph.region("GI");

    const FrameGraphImageId probes = trace_probes(framegraph, gbuffer);


    GIPass pass;
    pass.probes = probes;
    pass.gi = fetch_probes(framegraph, gbuffer, probes);
    pass.gi = debug_probes(framegraph, gbuffer, pass.gi, probes);
    return pass;
}

}

