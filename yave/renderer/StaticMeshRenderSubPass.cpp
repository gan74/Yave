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

#include "StaticMeshRenderSubPass_custom.h"

namespace yave {

StaticMeshRenderSubPass StaticMeshRenderSubPass::create(FrameGraphPassBuilder& builder, const SceneView& view, core::Vector<ecs::EntityId>&& ids, core::Span<core::String> tags) {
    y_profile();

    const ecs::EntityWorld& world = view.world();
    const RendererSystem* static_meshes = world.find_system<RendererSystem>();

    if(!static_meshes || ids.is_empty()) {
        return {};
    }

    usize batch_count = 0;
    {
        y_profile_zone("counting material");
        Y_TODO(optimize?)
        auto query = world.query<StaticMeshComponent>(ids, tags);
        for(const auto& [mesh] : query.components()) {
            batch_count += mesh.materials().size();
        }
    }


    static const PipelineStage stage = PipelineStage::VertexBit | PipelineStage::FragmentBit;
    const i32 descriptor_set_index = builder.next_descriptor_set_index();

    const auto indices_buffer = builder.declare_typed_buffer<math::Vec2ui>(batch_count);
    builder.map_buffer(indices_buffer);

    builder.add_external_input(Descriptor(static_meshes->transform_buffer()), stage, descriptor_set_index);
    builder.add_external_input(Descriptor(material_allocator().material_buffer()), stage, descriptor_set_index);
    builder.add_storage_input(indices_buffer, stage, descriptor_set_index);

    StaticMeshRenderSubPass pass;
    pass.scene_view = view;
    pass.ids = std::move(ids);
    pass.tags = tags;
    pass.indices_buffer = indices_buffer;
    pass.descriptor_set_index = descriptor_set_index;
    return pass;
}

void StaticMeshRenderSubPass::render(RenderPassRecorder& render_pass, const FrameGraphPass* pass) const {
    const MaterialTemplate* previous = nullptr;
    render_custom(render_pass, pass, [&](ecs::EntityId, const StaticMeshComponent&, const MeshDrawCommand& draw_cmd, const Material* material, u32 instance_index) {
        if(const MaterialTemplate* mat_template = material->material_template(); mat_template != previous) {
            const std::array<DescriptorSetBase, 2> desc_sets = {pass->descriptor_sets()[descriptor_set_index], texture_library().descriptor_set()};
            render_pass.bind_material_template(mat_template, desc_sets, true);
            previous = mat_template;
        }

        render_pass.draw(draw_cmd.vk_indirect_data(instance_index));
    });
}



}

