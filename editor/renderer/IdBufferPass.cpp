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

#include <yave/ecs/ecs.h>

#include <yave/systems/OctreeSystem.h>
#include <yave/systems/StaticMeshManagerSystem.h>
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
                        const FrameGraphMutableTypedBufferId<u32> id_buffer,
                        EditorPassFlags flags) {
    y_profile();

    const EditorWorld& world = current_world();
    y_debug_assert(&world == &scene_view.world());

    core::Vector<ecs::EntityId> visible;
    if((flags & EditorPassFlags::SelectionOnly) == EditorPassFlags::SelectionOnly) {
        visible = world.selected_entities();
    } else if(const OctreeSystem* octree_system = world.find_system<OctreeSystem>()) {
        const Camera& camera = scene_view.camera();
        visible = octree_system->octree().find_entities(camera.frustum(), camera.far_plane_dist());
    } else {
        visible = world.component_ids<StaticMeshComponent>();
    }


    const StaticMeshManagerSystem* static_meshes = world.find_system<StaticMeshManagerSystem>();
    const auto render_list = static_meshes->create_render_list(visible);
    const auto batches = render_list.batches();

    recorder.set_main_descriptor_set(pass->descriptor_sets()[0]);

    TypedBuffer<math::Vec2ui, BufferUsage::StorageBit, MemoryType::CpuVisible> indices(render_list.batches().size());

    {
        auto id_mapping = pass->resources().map_buffer(id_buffer);
        auto mapping = indices.map(MappingAccess::WriteOnly);
        for(usize i = 0; i != batches.size(); ++i) {
            mapping[i] = math::Vec2ui(batches[i].transform_index, batches[i].material_index);
            id_mapping[i] = batches[i].id.index();
        }
    }

    DescriptorSet set(std::array{
        Descriptor(static_meshes->transform_buffer()),
        Descriptor(indices),
    });

    recorder.bind_mesh_buffers(mesh_allocator().mesh_buffers());
    recorder.bind_material_template(resources()[EditorResources::IdMaterialTemplate], set, true);

    for(usize i = 0; i != batches.size(); ++i) {
        recorder.draw(batches[i].draw_cmd.vk_indirect_data(u32(i)));
    }
}

IdBufferPass IdBufferPass::create(FrameGraph& framegraph, SceneView view, const math::Vec2ui& size, EditorPassFlags flags) {
    static constexpr ImageFormat depth_format = VK_FORMAT_D32_SFLOAT;
    static constexpr ImageFormat id_format = VK_FORMAT_R32_UINT;

    FrameGraphPassBuilder builder = framegraph.add_pass("ID pass");

    const auto depth = builder.declare_image(depth_format, size);
    const auto id = builder.declare_image(id_format, size);

    const auto id_buffer = builder.declare_typed_buffer<u32>(StaticMeshManagerSystem::max_transforms);
    const auto camera_buffer = builder.declare_typed_buffer<uniform::Camera>(1);

    IdBufferPass pass;
    pass.scene_view = view;
    pass.depth = depth;
    pass.id = id;

    builder.map_buffer(id_buffer);
    builder.map_buffer(camera_buffer);

    builder.add_uniform_input(camera_buffer);
    builder.add_storage_input(id_buffer);
    builder.add_depth_output(depth);
    builder.add_color_output(id);
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        auto camera_mapping = self->resources().map_buffer(camera_buffer);
        camera_mapping[0] = view.camera();

        render_world(render_pass, self, view, id_buffer, flags);
    });

    return pass;
}

}

