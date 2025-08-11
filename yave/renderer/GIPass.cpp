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


static FrameGraphImageId debug_probes(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId lit, FrameGraphVolumeId grid) {
    const math::Vec3ui grid_size = framegraph.volume_size(grid);

    FrameGraphPassBuilder builder = framegraph.add_pass("Probe debug pass");

    const auto depth = builder.declare_copy(gbuffer.depth);
    const auto color = builder.declare_copy(lit);

    // const math::Vec2ui size = framegraph.image_size(lit);
    // const auto depth = builder.declare_image(VK_FORMAT_D32_SFLOAT, size);
    // const auto color = builder.declare_image(VK_FORMAT_R16G16B16A16_SFLOAT, size)

    builder.add_depth_output(depth);
    builder.add_color_output(color);

#if 0
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_uniform_input(grid);
    builder.add_inline_input(u32(framegraph.frame_id()));
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        const auto* material = device_resources()[DeviceResources::DebugProbesTemplate];
        render_pass.bind_material_template(material, self->descriptor_set());

        const StaticMesh& sphere = *device_resources()[DeviceResources::SimpleSphereMesh];
        render_pass.draw(sphere.draw_data(), u32(grid_size.x() * grid_size.y() * grid_size.z()));
    });
#endif

    const SceneView& scene_view = gbuffer.scene_pass.scene_view;
    const TLAS& tlas = scene_view.scene()->tlas();

    builder.add_uniform_input(grid);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_external_input(Descriptor(material_allocator().material_buffer()));
    builder.add_external_input(Descriptor(tlas));
    builder.add_inline_input(u32(framegraph.frame_id()));
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        const std::array<DescriptorSetProxy, 2> desc_sets = {
            self->descriptor_set(),
            texture_library().descriptor_set()
        };

        const auto* material = device_resources()[DeviceResources::DebugProbesTemplate];
        render_pass.bind_material_template(material, desc_sets);

        const StaticMesh& sphere = *device_resources()[DeviceResources::SimpleSphereMesh];
        render_pass.draw(sphere.draw_data(), u32(grid_size.x() * grid_size.y() * grid_size.z()));
    });

    return color;
}



GIPass GIPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId lit) {
    if(!raytracing_enabled()) {
        return {lit};
    }

    const auto region = framegraph.region("GI");

    // const SceneView& scene_view = gbuffer.scene_pass.scene_view;
    // const TLAS& tlas = scene_view.scene()->tlas();

    static const math::Vec3ui grid_size = math::Vec3ui(64);
    const math::Vec2ui size = framegraph.image_size(lit);
    FrameGraphMutableVolumeId grid;
    FrameGraphMutableTypedBufferId<u32> count;

    {
        FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Probe placing pass");

        grid = builder.declare_volume(VK_FORMAT_R32_UINT, grid_size);

        builder.add_uniform_input(gbuffer.scene_pass.camera);
        builder.add_uniform_input(gbuffer.depth);
        builder.add_uniform_input(gbuffer.normal);
        builder.add_storage_output(grid);
        builder.add_inline_input(u32(framegraph.frame_id()));
        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            const auto& program = device_resources()[DeviceResources::PlaceProbesProgram];
            recorder.dispatch_threads(program, size, self->descriptor_set());
        });
    }

#if 0
    {
        FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Probe clean pass");

        count = builder.declare_typed_buffer<u32>();

        builder.add_storage_output(count);
        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            const auto& program = device_resources()[DeviceResources::CleanProbesProgram];
            recorder.dispatch(program, math::Vec3ui(1), self->descriptor_set());
        });
    }

    {
        FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Probe gather pass");

        builder.add_uniform_input(grid);
        builder.add_uniform_input(gbuffer.scene_pass.camera);
        builder.add_external_input(Descriptor(material_allocator().material_buffer()));
        builder.add_external_input(Descriptor(tlas));
        builder.add_storage_output(count);
        builder.add_inline_input(u32(framegraph.frame_id()));
        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            const std::array<DescriptorSetProxy, 2> desc_sets = {
                self->descriptor_set(),
                texture_library().descriptor_set()
            };
            const auto& program = device_resources()[DeviceResources::GatherProbesProgram];
            recorder.dispatch_threads(program, grid_size, desc_sets);
        });
    }
#endif

    GIPass pass;
    pass.lit = debug_probes(framegraph, gbuffer, lit, grid);
    return pass;
}

}

