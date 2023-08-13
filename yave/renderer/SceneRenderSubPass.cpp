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
#include <yave/systems/StaticMeshRendererSystem.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/ecs/EntityWorld.h>

#include <y/utils/format.h>

namespace yave {

template<typename T>
static core::Vector<ecs::EntityId> visible_entities(const SceneView& scene_view) {
    const ecs::EntityWorld& world = scene_view.world();

    if(const OctreeSystem* octree_system = world.find_system<OctreeSystem>()) {
        const Camera& camera = scene_view.camera();
        Y_TODO(camera far does not work properly)
        return octree_system->octree().find_entities(camera.frustum(), camera.far_plane_dist());
    }

    return core::Vector<ecs::EntityId>(world.component_ids<T>());
}


SceneRenderSubPass SceneRenderSubPass::create(FrameGraphPassBuilder& builder, const SceneView& view) {
    auto camera_buffer = builder.declare_typed_buffer<uniform::Camera>();

    const std::array tags = {ecs::tags::not_hidden};

    SceneRenderSubPass pass;
    pass.scene_view = view;
    pass.camera_buffer = camera_buffer;
    pass.static_meshes_sub_pass = StaticMeshRenderSubPass::create(builder, view, visible_entities<StaticMeshComponent>(view), tags);
    pass.main_descriptor_set_index = builder.next_descriptor_set_index();

    builder.add_uniform_input(camera_buffer, PipelineStage::None, pass.main_descriptor_set_index);
    builder.map_buffer(camera_buffer);

    return pass;
}

void SceneRenderSubPass::render(RenderPassRecorder& render_pass, const FrameGraphPass* pass) const {
    {
        auto camera_mapping = pass->resources().map_buffer(camera_buffer);
        camera_mapping[0] = scene_view.camera();
    }

    render_pass.set_main_descriptor_set(pass->descriptor_sets()[main_descriptor_set_index]);
    static_meshes_sub_pass.render(render_pass, pass);
}

}

