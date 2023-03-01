/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include "ISMPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/shaders/ComputeProgram.h>

#include <yave/systems/SurfelCacheSystem.h>

#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/ecs/EntityWorld.h>

#include <y/utils/log.h>
#include <y/utils/format.h>


namespace yave {


const math::Vec3ui screen_division = math::Vec3ui(32, 32, 1);

static math::Vec2ui compute_probe_count(const math::Vec2ui& size) {
    math::Vec2ui probe_count;
    for(usize i = 0; i != 2; ++i) {
        probe_count[i] = size[i] / screen_division[i] + !!(size[i] % screen_division[i]);
    }
    return probe_count;
}


ISMPass ISMPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer) {
    const SurfelCacheSystem* cache = gbuffer.scene_pass.scene_view.world().find_system<SurfelCacheSystem>();
    if(!cache) {
        return {};
    }

    const auto region = framegraph.region("ISM");

    const u32 instance_count = cache->instance_count();

    const math::Vec2ui gbuffer_size = framegraph.image_size(gbuffer.depth);
    const math::Vec2ui probe_count = compute_probe_count(gbuffer_size);
    const usize total_probe_count = probe_count.x() * probe_count.y();

    FrameGraphMutableVolumeId ism;
    FrameGraphMutableTypedBufferId<math::Vec4> probes;


    {
        FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Probe gen pass");

        probes = builder.declare_typed_buffer<math::Vec4>(total_probe_count);

        builder.add_uniform_input(gbuffer.depth);
        builder.add_uniform_input(gbuffer.color);
        builder.add_uniform_input(gbuffer.normal);
        builder.add_uniform_input(gbuffer.scene_pass.camera_buffer);
        builder.add_storage_output(probes);
        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            const ComputeProgram& program = device_resources().from_file("generate_probes.comp.spv");
            y_debug_assert(program.local_size() == screen_division);
            recorder.dispatch(program, math::Vec3ui(probe_count, 1), self->descriptor_sets());
        });
    }

    {
        FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("ISM clear pass");

        const math::Vec2ui probe_size = math::Vec2ui(32, 32);
        ism = builder.declare_volume(VK_FORMAT_R64_UINT, math::Vec3ui(probe_size, total_probe_count));

        builder.add_storage_output(ism);
        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            const ComputeProgram& program = device_resources().from_file("clear_probes.comp.spv");
            recorder.dispatch_size(program, math::Vec3ui(probe_size, total_probe_count), self->descriptor_sets());
        });
    }

    {
        FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("ISM pass");

        builder.add_storage_output(ism);
        builder.add_uniform_input(gbuffer.scene_pass.camera_buffer);
        builder.add_external_input(cache->surfel_buffer());
        builder.add_external_input(cache->instance_buffer());
        builder.add_storage_input(probes);
        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            const ComputeProgram& program = device_resources().from_file("splat_surfels.comp.spv");
            recorder.dispatch(program, math::Vec3ui(instance_count, total_probe_count, 1), self->descriptor_sets());
        });
    }

    {
        FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("ISM filling pass");

        builder.add_storage_output(ism);
        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            const ComputeProgram& program = device_resources().from_file("fill_probes.comp.spv");
            recorder.dispatch(program, math::Vec3ui(1, 1, total_probe_count), self->descriptor_sets());
        });
    }

    ISMPass pass;
    pass.ism = ism;
    pass.probes = probes;
    return pass;
}

}

