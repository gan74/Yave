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

#include <yave/systems/SurfelCacheSystem.h>

#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/ecs/EntityWorld.h>

#include <y/utils/log.h>
#include <y/utils/format.h>


namespace yave {

ISMPass ISMPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer) {
    const SurfelCacheSystem* cache = gbuffer.scene_pass.scene_view.world().find_system<SurfelCacheSystem>();
    if(!cache) {
        return {};
    }

    const auto region = framegraph.region("ISM");

    FrameGraphPassBuilder clear_builder = framegraph.add_pass("ISM clear pass");

    const math::Vec2ui size = math::Vec2ui(128, 128);
    const auto ism = clear_builder.declare_image(VK_FORMAT_R32_UINT, size);

    clear_builder.add_color_output(ism);
    clear_builder.set_render_func([=](RenderPassRecorder&, const FrameGraphPass*) {
    });


    const u32 surfel_count = cache->surfel_count();
    const u32 instance_count = cache->instance_count();

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("ISM pass");

    builder.add_storage_output(ism);
    builder.add_uniform_input(gbuffer.scene_pass.camera_buffer);
    builder.add_external_input(cache->surfel_buffer());
    builder.add_external_input(cache->instance_buffer());
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        log_msg(fmt("Splatting % surfels for % instances", surfel_count, instance_count));

        if(false) {
            const ComputeProgram& program = device_resources().from_file("ism_splat.comp.spv");
            recorder.dispatch_size(program, math::Vec2ui(surfel_count, 1), self->descriptor_sets());
        } else {
            const ComputeProgram& program = device_resources().from_file("splat_surfels.comp.spv");
            recorder.dispatch(program, math::Vec3ui(instance_count, 1, 1), self->descriptor_sets());
        }
    });

    ISMPass pass;
    pass.ism = ism;
    return pass;
}

}

