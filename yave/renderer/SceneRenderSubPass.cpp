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

#include "SceneRenderSubPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/systems/OctreeSystem.h>
#include <yave/systems/StaticMeshManagerSystem.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/ecs/EntityWorld.h>

#include <y/utils/format.h>

namespace yave {

SceneRenderSubPass SceneRenderSubPass::create(FrameGraphPassBuilder& builder, const SceneView& view) {
    auto camera_buffer = builder.declare_typed_buffer<Renderable::CameraData>();

    SceneRenderSubPass pass;
    pass.scene_view = view;
    pass.descriptor_set_index = builder.next_descriptor_set_index();
    pass.camera_buffer = camera_buffer;

    builder.add_uniform_input(camera_buffer, PipelineStage::None, pass.descriptor_set_index);
    builder.map_buffer(camera_buffer);

    return pass;
}


static usize render_world(const SceneRenderSubPass* sub_pass, RenderPassRecorder& recorder, const FrameGraphPass* pass, usize index = 0) {
    y_profile();

    const auto region = recorder.region("Scene");

    const ecs::EntityWorld& world = sub_pass->scene_view.world();
    const Camera& camera = sub_pass->scene_view.camera();

    const auto& descriptor_set = pass->descriptor_sets()[sub_pass->descriptor_set_index];
    recorder.set_main_descriptor_set(descriptor_set);

    const StaticMeshManagerSystem* manager = world.find_system<StaticMeshManagerSystem>();
    recorder.bind_per_instance_attrib_buffers(manager->transform_buffer());

    auto render_query = [&](auto query) {
        for(const auto& [mesh] : query.components()) {
            mesh.render(recorder);
        }
    };

    const std::array tags = {ecs::tags::not_hidden};
    if(const OctreeSystem* octree_system = world.find_system<OctreeSystem>()) {
        const core::Vector<ecs::EntityId> visible = octree_system->octree().find_entities(camera.frustum(), camera.far_plane_dist());
        render_query(world.query<StaticMeshComponent>(visible, tags));
    } else {
        render_query(world.query<StaticMeshComponent>(tags));
    }

    y_profile_msg(fmt_c_str("% meshes", index));

    return index;
}

void SceneRenderSubPass::render(RenderPassRecorder& recorder, const FrameGraphPass* pass) const {
    // fill render data
    auto camera_mapping = pass->resources().map_buffer(camera_buffer);
    camera_mapping[0] = scene_view.camera();

    if(scene_view.has_world()) {
        render_world(this, recorder, pass, 0);
    }
}

}

