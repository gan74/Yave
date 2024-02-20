/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include "StaticMeshRenderer.h"

#include <yave/scene/SceneView.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/material/Material.h>
#include <yave/components/StaticMeshComponent.h>

#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/device/MeshAllocator.h>
#include <yave/graphics/device/MaterialAllocator.h>
#include <yave/graphics/images/TextureLibrary.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/framegraph/FrameGraphPassBuilder.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>


namespace yave {

template<typename Q>
static void render_static_meshes(Q query,
                                 RenderPassRecorder& render_pass,
                                 TypedSubBuffer<VkDrawIndexedIndirectCommand, BufferUsage::IndirectBit> indirect,
                                 BufferMapping<VkDrawIndexedIndirectCommand> indirect_mapping,
                                 BufferMapping<math::Vec2ui> indices_mapping,
                                 const DescriptorSet& pass_set) {
    y_profile();

    u32 last = 0;
    u32 index = 0;
    const MaterialTemplate* previous = nullptr;
    auto flush_mat = [&](const Material* material) {
        const MaterialTemplate* mat_template = material ? material->material_template() : nullptr;
        if(mat_template != previous) {
            if(previous) {
                const std::array<DescriptorSetBase, 2> desc_sets = {pass_set, texture_library().descriptor_set()};
                render_pass.bind_material_template(previous, desc_sets, true);

                render_pass.draw_indirect(TypedSubBuffer<VkDrawIndexedIndirectCommand, BufferUsage::IndirectBit>(indirect, index - last, last));
                last = index;
            }
            previous = mat_template;
        }
    };


    for(const auto& [id, comp] : query.id_components()) {
        const auto& [tr, mesh] = comp;
        const u32 transform_index = tr.transform_index();

        if(!mesh.mesh() || transform_index == u32(-1)) {
            continue;
        }

        const auto materials = mesh.materials();
        if(materials.size() == 1) {
            if(const Material* mat = materials[0].get()) {
                flush_mat(mat);
                indirect_mapping[index] = mesh.mesh()->draw_command().vk_indirect_data(index);
                indices_mapping[index] = math::Vec2ui(
                    transform_index,
                    mat->draw_data().index()
                );

                ++index;
            }
        } else {
            for(usize i = 0; i != materials.size(); ++i) {
                if(const Material* mat = materials[i].get()) {
                    flush_mat(mat);
                    indirect_mapping[index] = mesh.mesh()->sub_meshes()[i].vk_indirect_data(index);
                    indices_mapping[index] = math::Vec2ui(
                        transform_index,
                        mat->draw_data().index()
                    );

                    ++index;
                }
            }
        }
    }
    flush_mat(nullptr);
}

template<typename Q>
static void render_static_meshes_id(Q query,
                                    RenderPassRecorder& render_pass,
                                    TypedSubBuffer<VkDrawIndexedIndirectCommand, BufferUsage::IndirectBit> indirect,
                                    BufferMapping<VkDrawIndexedIndirectCommand> indirect_mapping,
                                    BufferMapping<math::Vec2ui> indices_mapping,
                                    const DescriptorSet& pass_set) {
    y_profile();

    render_pass.bind_material_template(device_resources()[DeviceResources::IdMaterialTemplate], pass_set, true);

    u32 index = 0;
    for(const auto& [id, comp] : query.id_components()) {
        const auto& [tr, mesh] = comp;
        const u32 transform_index = tr.transform_index();

        if(!mesh.mesh() || transform_index == u32(-1)) {
            continue;
        }

        indirect_mapping[index] = mesh.mesh()->draw_command().vk_indirect_data(index);
        indices_mapping[index] = math::Vec2ui(
            transform_index,
            id.index()
        );

        ++index;
    }

    render_pass.draw_indirect(TypedSubBuffer<VkDrawIndexedIndirectCommand, BufferUsage::IndirectBit>(indirect, index, 0_uu));
}



StaticMeshRenderer::RenderFunc StaticMeshRenderer::prepare_render(FrameGraphPassBuilder& builder, const SceneView& view, core::Span<ecs::EntityId> ids, PassType pass_type) const {
    y_profile();

    const ecs::EntityWorld* world = &view.world();
    const RendererSystem* renderer = parent();

    usize batch_count = 0;
    {
        y_profile_zone("counting material");
        Y_TODO(optimize?)
        auto query = world->query<StaticMeshComponent>(ids);
        for(const auto& [mesh] : query.components()) {
            batch_count += mesh.materials().size();
        }
    }

    if(!batch_count) {
        return {};
    }

    static const PipelineStage stage = PipelineStage::VertexBit | PipelineStage::FragmentBit;
    const i32 descriptor_set_index = builder.next_descriptor_set_index();

    const auto indices_buffer = builder.declare_typed_buffer<math::Vec2ui>(batch_count);
    builder.map_buffer(indices_buffer);

    const auto indirect_buffer = builder.declare_typed_buffer<VkDrawIndexedIndirectCommand>(batch_count);
    builder.map_buffer(indirect_buffer);

    builder.add_external_input(Descriptor(renderer->transform_buffer()), stage, descriptor_set_index);
    builder.add_external_input(Descriptor(material_allocator().material_buffer()), stage, descriptor_set_index);
    builder.add_storage_input(indices_buffer, stage, descriptor_set_index);
    builder.add_indrect_input(indirect_buffer);

    return [=](RenderPassRecorder& render_pass, const FrameGraphPass* pass) {
        auto query = world->query<TransformableComponent, StaticMeshComponent>(ids);
        if(query.is_empty()) {
            return;
        }

        const auto region = render_pass.region("static meshes");

        render_pass.bind_mesh_buffers(mesh_allocator().mesh_buffers());

        const auto& pass_set = pass->descriptor_sets()[descriptor_set_index];

        switch(pass_type) {
            case PassType::Depth:
            case PassType::GBuffer:
                render_static_meshes(
                    std::move(query),
                    render_pass,
                    pass->resources().buffer<BufferUsage::IndirectBit>(indirect_buffer),
                    pass->resources().map_buffer(indirect_buffer),
                    pass->resources().map_buffer(indices_buffer),
                    pass_set
                );
            break;

            case PassType::Id:
                render_static_meshes_id(
                    std::move(query),
                    render_pass,
                    pass->resources().buffer<BufferUsage::IndirectBit>(indirect_buffer),
                    pass->resources().map_buffer(indirect_buffer),
                    pass->resources().map_buffer(indices_buffer),
                    pass_set
                );
            break;
        }
    };
}

}

