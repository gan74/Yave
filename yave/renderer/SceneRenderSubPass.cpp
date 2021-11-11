/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/ecs/EntityWorld.h>

namespace yave {

SceneRenderSubPass SceneRenderSubPass::create(FrameGraphPassBuilder& builder, const SceneView& view) {
    auto camera_buffer = builder.declare_typed_buffer<Renderable::CameraData>();
    const auto transform_buffer = builder.declare_typed_buffer<math::Transform<>>(max_batch_size);

    SceneRenderSubPass pass;
    pass.scene_view = view;
    pass.descriptor_set_index = builder.next_descriptor_set_index();
    pass.camera_buffer = camera_buffer;
    pass.transform_buffer = transform_buffer;

    builder.add_uniform_input(camera_buffer, pass.descriptor_set_index);
    builder.add_attrib_input(transform_buffer);
    builder.map_update(camera_buffer);
    builder.map_update(transform_buffer);

    return pass;
}


static usize render_world(const SceneRenderSubPass* sub_pass, RenderPassRecorder& recorder, const FrameGraphPass* pass, usize index = 0) {
    y_profile();

    const auto region = recorder.region("Scene");

    const ecs::EntityWorld& world = sub_pass->scene_view.world();
    const Camera& camera = sub_pass->scene_view.camera();

    auto transform_mapping = pass->resources().mapped_buffer(sub_pass->transform_buffer);
    const auto transforms = pass->resources().buffer<BufferUsage::AttributeBit>(sub_pass->transform_buffer);
    const auto& descriptor_set = pass->descriptor_sets()[sub_pass->descriptor_set_index];

    recorder.bind_attrib_buffers({}, {transforms});

    core::Vector<ecs::EntityId> visible;
    const OctreeSystem* octree_system = world.find_system<OctreeSystem>();
    if(octree_system) {
        visible = octree_system->octree().find_entities(camera.frustum());
    }
    const auto entities = octree_system
        ? world.query<TransformableComponent, StaticMeshComponent>(visible)
        : world.query<TransformableComponent, StaticMeshComponent>();

    for(const auto& [tr, mesh] : entities.components()) {
        transform_mapping[index] = tr.transform();
        mesh.render(recorder, Renderable::SceneData{descriptor_set, u32(index)});
        ++index;
    }

    return index;
}

void SceneRenderSubPass::render(RenderPassRecorder& recorder, const FrameGraphPass* pass) const {
    // fill render data
    auto camera_mapping = pass->resources().mapped_buffer(camera_buffer);
    camera_mapping[0] = scene_view.camera();

    usize index = 0;
    if(scene_view.has_world()) {
        index = render_world(this, recorder, pass, index);
    }
}

}

