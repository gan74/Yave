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

struct StaticMeshBatch {
    const MaterialTemplate* material_template = nullptr;
    VkDrawIndexedIndirectCommand cmd = {};
    math::Vec2ui indices;
};



template<typename Q>
static void collect_batches(Q query, core::Vector<StaticMeshBatch>& batches) {
    y_profile();

    for(const auto& [tr, mesh] : query.components()) {
        const u32 transform_index = tr.transform_index();

        if(!mesh.mesh() || transform_index == u32(-1)) {
            continue;
        }

        const core::Span materials = mesh.materials();
        if(materials.size() == 1) {
            if(const Material* mat = materials[0].get()) {
                batches.emplace_back(
                    mat->material_template(),
                    mesh.mesh()->draw_command().vk_indirect_data(),
                    math::Vec2ui(transform_index, mat->draw_data().index())
                );
            }
        } else {
            for(usize i = 0; i != materials.size(); ++i) {
                if(const Material* mat = materials[i].get()) {
                    batches.emplace_back(
                        mat->material_template(),
                        mesh.mesh()->sub_meshes()[i].vk_indirect_data(),
                        math::Vec2ui(transform_index, mat->draw_data().index())
                    );
                }
            }
        }
    }

    {
        y_profile_zone("sort batches");
        std::sort(batches.begin(), batches.end(), [](const StaticMeshBatch& a, const StaticMeshBatch& b) { return a.material_template < b.material_template; });
    }

    {
        usize index = 0;
        for(StaticMeshBatch& batch : batches) {
            batch.cmd.firstInstance = u32(index++);
        }
    }
}

static void render_static_meshes(const core::Vector<StaticMeshBatch>& batches,
                                 RenderPassRecorder& render_pass,
                                 IndirectSubBuffer indirect,
                                 const DescriptorSet& pass_set) {
    y_profile();

    const std::array<DescriptorSetBase, 2> desc_sets = {pass_set, texture_library().descriptor_set()};

    usize offset = 0;
    const MaterialTemplate* prev_template = nullptr;
    for(usize i = 0; i != batches.size(); ++i) {
        const StaticMeshBatch& batch = batches[i];
        if(prev_template == batch.material_template) {
            continue;
        }

        prev_template = batch.material_template;
        const usize batch_size = i - offset;

        if(!batch_size) {
            continue;
        }

        render_pass.bind_material_template(prev_template, desc_sets, true);
        render_pass.draw_indirect(IndirectSubBuffer(indirect, batch_size, offset));

        offset = i;
    }
}






template<typename Q>
static void collect_batches_id(Q query, core::Vector<StaticMeshBatch>& batches) {
    y_profile();

    usize index = 0;
    for(const auto& [id, comp] : query.id_components()) {
        const auto& [tr, mesh] = comp;
        const u32 transform_index = tr.transform_index();

        if(!mesh.mesh() || transform_index == u32(-1)) {
            continue;
        }

        batches.emplace_back(
            nullptr,
            mesh.mesh()->draw_command().vk_indirect_data(index++),
            math::Vec2ui(transform_index, id.index())
        );
    }
}

static void render_static_meshes_id(RenderPassRecorder& render_pass,
                                    IndirectSubBuffer indirect,
                                    const DescriptorSet& pass_set) {
    y_profile();

    render_pass.bind_material_template(device_resources()[DeviceResources::IdMaterialTemplate], pass_set, true);
    render_pass.draw_indirect(indirect);
}




StaticMeshRenderer::RenderFunc StaticMeshRenderer::prepare_render(FrameGraphPassBuilder& builder, const SceneView& view, core::Span<ecs::EntityId> ids, PassType pass_type) const {
    y_profile();

    const ecs::EntityWorld* world = &view.world();
    const RendererSystem* renderer = parent();

    if(pass_type == PassType::Id) {
        return {};
    }

    // This is needed because std::function requires the lambda to be coyable
    // Might be fixed by std::move_only_function in C++23
    Y_TODO(fix in cpp23)
    auto batches = std::make_shared<core::Vector<StaticMeshBatch>>();
    {
        auto query = world->query<TransformableComponent, StaticMeshComponent>(ids);
        switch(pass_type) {
            case PassType::Depth:
            case PassType::GBuffer: 
                collect_batches(std::move(query), *batches);
            break;

            case PassType::Id:
                collect_batches_id(std::move(query), *batches);
            break;
        }
    }


    const usize batch_count = batches->size();
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
        if(batches->is_empty()) {
            return;
        }

        const auto region = render_pass.region("static meshes");

        {
            y_profile_zone("copy indirect data");

            auto indirect_mapping = pass->resources().map_buffer(indirect_buffer);
            auto indices_mapping = pass->resources().map_buffer(indices_buffer);

            usize index = 0;
            for(const StaticMeshBatch& batch : *batches) {
                indirect_mapping[index] = batch.cmd;
                indices_mapping[index] = batch.indices;
                ++index;
            }
        }

        render_pass.bind_mesh_buffers(mesh_allocator().mesh_buffers());
        const auto& pass_set = pass->descriptor_sets()[descriptor_set_index];

        const IndirectSubBuffer buffer = pass->resources().buffer<BufferUsage::IndirectBit>(indirect_buffer);

        switch(pass_type) {
            case PassType::Depth:
            case PassType::GBuffer: 
                render_static_meshes(*batches, render_pass, buffer, pass_set);
            break;

            case PassType::Id:
                render_static_meshes_id(render_pass, buffer, pass_set);
            break;
        }
    };
}

}

