/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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


namespace yave {

static const IBLProbe* find_probe(const SceneView& scene_view) {
    for(const SkyLightObject& obj : scene_view.scene()->sky_lights()) {
        if((obj.visibility_mask & scene_view.visibility_mask()) == 0) {
            continue;
        }

        const SkyLightComponent& sky = obj.component;
        if(const IBLProbe* probe = sky.probe().get()) {
            y_debug_assert(!probe->is_null());
            return probe;
        }
    }

    return device_resources().empty_probe().get();
}

static math::Vec2ui probe_grid_size(FrameGraph& framegraph, const GBufferPass& gbuffer) {
    //return math::Vec2ui(1, 1);

    const math::Vec2ui size = framegraph.image_size(gbuffer.depth);
    return divide_align(size, device_resources()[DeviceResources::PlaceProbesProgram].local_size().to<2>());
}

static FrameGraphMutableTypedBufferId<shader::GIProbe> place_probes(FrameGraph& framegraph, const GBufferPass& gbuffer) {
    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Place probes pass");

    const math::Vec2ui probe_grid = probe_grid_size(framegraph, gbuffer);

    const auto probe_buffer = builder.declare_typed_buffer<shader::GIProbe>(probe_grid.x() * probe_grid.y());

    builder.add_uniform_input(gbuffer.depth, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_storage_output(probe_buffer);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const ComputeProgram& program = device_resources()[DeviceResources::PlaceProbesProgram];
        recorder.dispatch(program, math::Vec3ui(probe_grid, 1), self->descriptor_sets());
    });

    return probe_buffer;
}

static FrameGraphMutableTypedBufferId<shader::GIProbe> propagate_probes(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphMutableTypedBufferId<shader::GIProbe> probes) {
    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Propagate probes pass");

    const math::Vec2ui probe_grid = probe_grid_size(framegraph, gbuffer);

    const auto probe_buffer = builder.declare_typed_buffer<shader::GIProbe>(probe_grid.x() * probe_grid.y());
    y_debug_assert(framegraph.buffer_size(probes) == framegraph.buffer_size(probe_buffer));

    builder.add_storage_input(probes);
    builder.add_storage_output(probe_buffer);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const ComputeProgram& program = device_resources()[DeviceResources::PropagateProbesProgram];
        recorder.dispatch(program, math::Vec3ui(probe_grid, 1), self->descriptor_sets());

    });

    return probe_buffer;
}

static void trace_probes(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId lit, FrameGraphMutableTypedBufferId<shader::GIProbe> probes) {
    const usize probe_count = framegraph.buffer_size(probes);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Trace probes pass");

    const TLAS& tlas = gbuffer.scene_pass.scene_view.scene()->tlas();

    builder.add_storage_output(probes);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_uniform_input(gbuffer.depth, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.color, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.normal, SamplerType::PointClamp);
    builder.add_uniform_input(lit);
    builder.add_external_input(*find_probe(gbuffer.scene_pass.scene_view));
    builder.add_descriptor_binding(Descriptor(tlas));
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const ComputeProgram& program = device_resources()[DeviceResources::TraceProbesProgram];
        recorder.dispatch(program, math::Vec3ui(1, u32(probe_count), 1), self->descriptor_sets());
    });
}


static FrameGraphImageId debug_probes(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId lit, FrameGraphTypedBufferId<shader::GIProbe> probes) {
#if 0
    const usize probe_count = framegraph.buffer_size(probes);

    FrameGraphPassBuilder builder = framegraph.add_pass("Probe debug pass");

    const auto depth = builder.declare_copy(gbuffer.depth);
    const auto color = builder.declare_copy(lit);

    builder.add_depth_output(depth);
    builder.add_color_output(color);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_storage_input(probes);
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        const auto* material = device_resources()[DeviceResources::ProbeDebugTemplate];
        render_pass.bind_material_template(material, self->descriptor_sets());

        const StaticMesh& sphere = *device_resources()[DeviceResources::SimpleSphereMesh];
        render_pass.draw(sphere.draw_data(), u32(probe_count));
    });

    return color;
#else
    const math::Vec2ui size = framegraph.image_size(lit);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("GI debug pass");

    const auto color = builder.declare_image(VK_FORMAT_R16G16B16A16_SFLOAT, size);

    builder.add_uniform_input(gbuffer.depth, SamplerType::PointClamp);
    builder.add_uniform_input(gbuffer.color);
    builder.add_uniform_input(gbuffer.normal);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_storage_input(probes);
    builder.add_storage_output(color);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const ComputeProgram& program = device_resources()[DeviceResources::DebugGIprogram];
        recorder.dispatch_threads(program, size, self->descriptor_sets());
    });

    return color;
#endif
}


GIPass GIPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId lit) {
    // return {lit};

    const auto region = framegraph.region("GI");

    auto probes = place_probes(framegraph, gbuffer);

    if(raytracing_enabled()) {
        trace_probes(framegraph, gbuffer, lit, probes);
        // probes = propagate_probes(framegraph, gbuffer, probes);
    } else {
        log_msg("No ray tracing: GI probe update disabled", Log::Warning);
    }

    GIPass pass;
    pass.lit = debug_probes(framegraph, gbuffer, lit, probes);
    return pass;
}

}

