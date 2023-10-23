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

#include "IdBufferPass.h"

#include <editor/EditorWorld.h>
#include <editor/EditorResources.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/device/MeshAllocator.h>

#include <yave/systems/OctreeSystem.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/meshes/MeshData.h>

#include <yave/renderer/StaticMeshRenderSubPass_custom.h>



namespace editor {

template<typename T>
static core::Vector<ecs::EntityId> visible_entities(const SceneView& scene_view, EditorPassFlags flags) {
    y_profile();

    const EditorWorld& world = current_world();
    y_debug_assert(&world == &scene_view.world());

    const bool selection = (flags & (EditorPassFlags::SelectionOnly | EditorPassFlags::SelectionAndChildren)) != EditorPassFlags::None;
    if(selection) {
        auto ids = core::Vector<ecs::EntityId>::from_range(world.selected_entities());
        if((flags & EditorPassFlags::SelectionAndChildren) != EditorPassFlags::None) {
            for(usize i = 0; i != ids.size(); ++i) {
                for(const ecs::EntityId child : world.children(ids[i])) {
                    if(!world.is_selected(child)) {
                        ids << child;
                    }
                }
            }
        }
        return ids;
    } else if(const OctreeSystem* octree_system = world.find_system<OctreeSystem>()) {
        return octree_system->find_entities(scene_view.camera());
    }

    return core::Vector<ecs::EntityId>(world.component_set<T>().ids());
}

IdBufferPass IdBufferPass::create(FrameGraph& framegraph, const SceneView& view, const math::Vec2ui& size, EditorPassFlags flags) {
    static constexpr ImageFormat depth_format = VK_FORMAT_D32_SFLOAT;
    static constexpr ImageFormat id_format = VK_FORMAT_R32_UINT;

    FrameGraphPassBuilder builder = framegraph.add_pass("ID pass");

    const std::array tags = {ecs::tags::not_hidden};
    auto static_mesh_sub_pass = StaticMeshRenderSubPass::create(builder, view, visible_entities<StaticMeshComponent>(view, flags), tags);
    const usize buffer_size = static_mesh_sub_pass.indices_buffer.is_valid() ? framegraph.buffer_size(static_mesh_sub_pass.indices_buffer) : 1;

    static const PipelineStage stage = PipelineStage::VertexBit | PipelineStage::FragmentBit;
    const i32 main_descriptor_set_index = builder.next_descriptor_set_index();

    const auto depth = builder.declare_image(depth_format, size);
    const auto id = builder.declare_image(id_format, size);

    const auto id_buffer = builder.declare_typed_buffer<u32>(buffer_size);
    const auto camera = builder.declare_typed_buffer<uniform::Camera>(1);

    builder.map_buffer(id_buffer);
    builder.map_buffer(camera, uniform::Camera(view.camera()));

    builder.add_uniform_input(camera, stage, main_descriptor_set_index);
    builder.add_storage_input(id_buffer, stage, main_descriptor_set_index);
    builder.add_depth_output(depth);
    builder.add_color_output(id);
    builder.set_render_func([=, static_meshes = std::move(static_mesh_sub_pass)](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        {
            auto id_mapping = self->resources().map_buffer(id_buffer);

            if(static_meshes.descriptor_set_index >= 0) {
                render_pass.set_main_descriptor_set(self->descriptor_sets()[main_descriptor_set_index]);
                render_pass.bind_material_template(resources()[EditorResources::IdMaterialTemplate], self->descriptor_sets()[static_meshes.descriptor_set_index], true);

                static_meshes.render_custom(render_pass, self, [&](ecs::EntityId id, const StaticMeshComponent&, const MeshDrawCommand& draw_cmd, const Material*, u32 instance_index) {
                    id_mapping[instance_index] = id.index();
                    render_pass.draw(draw_cmd.vk_indirect_data(instance_index));
                });
            }
        }
    });


    IdBufferPass pass;
    pass.scene_view = view;
    pass.depth = depth;
    pass.id = id;
    return pass;
}

}

