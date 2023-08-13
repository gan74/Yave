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

#include <yave/ecs/ecs.h>

#include <yave/systems/OctreeSystem.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/meshes/MeshData.h>


Y_TODO(merge with scene sub pass?)

// mostly copied from SceneRednerSubPass and EditorPass
namespace editor {

static void render_world(RenderPassRecorder& recorder, const FrameGraphPass* pass,
                          const SceneView& scene_view,
                          const FrameGraphMutableTypedBufferId<math::Transform<>> transform_buffer,
                          const FrameGraphMutableTypedBufferId<u32> id_buffer,
                          EditorPassFlags flags) {
    y_profile();

    const EditorWorld& world = current_world();
    y_debug_assert(&world == &scene_view.world());

    auto transform_mapping = pass->resources().map_buffer(transform_buffer);
    const auto transforms = pass->resources().buffer<BufferUsage::AttributeBit>(transform_buffer);

    auto id_mapping = pass->resources().map_buffer(id_buffer);
    const auto ids = pass->resources().buffer<BufferUsage::AttributeBit>(id_buffer);

    recorder.bind_per_instance_attrib_buffers(std::array{transforms, ids});
    recorder.bind_material_template(resources()[EditorResources::IdMaterialTemplate], pass->descriptor_sets()[0]);

    bool use_entity_list = false;
    core::Vector<ecs::EntityId> entities;

    if((flags & EditorPassFlags::SelectionOnly) == EditorPassFlags::SelectionOnly) {
        const auto selected = world.selected_entities();
        entities.assign(selected.begin(), selected.end());
        use_entity_list = true;
    } else {
        if(const OctreeSystem* octree_system = world.find_system<OctreeSystem>()) {
            entities = octree_system->find_entities(scene_view.camera());
            use_entity_list = true;
        }
    }

    auto render_query = [&](auto query) {
        usize index = 0;
        for(auto&& [id, comp] : query) {
            const auto& [tr, mesh] = comp;
            transform_mapping[index] = tr.transform();
            id_mapping[index] = id.index();
            mesh.render_mesh(recorder, u32(index));
            ++index;
        }
    };

    if(use_entity_list) {
        render_query(world.query<TransformableComponent, StaticMeshComponent>(entities));
    } else {
        render_query(world.query<TransformableComponent, StaticMeshComponent>());
    }
}

IdBufferPass IdBufferPass::create(FrameGraph& framegraph, const SceneView& view, const math::Vec2ui& size, EditorPassFlags flags) {
    static constexpr ImageFormat depth_format = VK_FORMAT_D32_SFLOAT;
    static constexpr ImageFormat id_format = VK_FORMAT_R32_UINT;

    const usize buffer_size = view.world().components<TransformableComponent>().size();

    FrameGraphPassBuilder builder = framegraph.add_pass("ID pass");

    const auto transform_buffer = builder.declare_typed_buffer<math::Transform<>>(buffer_size);
    const auto id_buffer = builder.declare_typed_buffer<u32>(buffer_size);

    const auto depth = builder.declare_image(depth_format, size);
    const auto id = builder.declare_image(id_format, size);

    IdBufferPass pass;
    pass.scene_view = view;
    pass.depth = depth;
    pass.id = id;

    builder.add_inline_input(InlineDescriptor(view.camera().viewproj_matrix()));
    builder.add_attrib_input(transform_buffer);
    builder.add_attrib_input(id_buffer);
    builder.map_buffer(transform_buffer);
    builder.map_buffer(id_buffer);

    builder.add_depth_output(depth);
    builder.add_color_output(id);
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        render_world(render_pass, self, view, transform_buffer, id_buffer, flags);
    });

    return pass;
}

}

