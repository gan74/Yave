/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include "Scene.h"
#include "SceneVisibility.h"

#include <yave/meshes/StaticMesh.h>
#include <yave/material/Material.h>

#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/device/MeshAllocator.h>
#include <yave/graphics/device/MaterialAllocator.h>
#include <yave/graphics/images/TextureLibrary.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/framegraph/FrameGraphPassBuilder.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/framegraph/FrameGraphPass.h>

namespace yave {

struct StaticMeshBatch {
    const MaterialTemplate* material_template = nullptr;
    VkDrawIndexedIndirectCommand cmd = {};
    shader::MeshObject mesh_object;
};


static void collect_batches(core::Span<const StaticMeshObject*> meshes, core::Vector<StaticMeshBatch>& batches) {
    y_profile();

    batches.set_min_capacity(meshes.size() * 4);
    for(const StaticMeshObject* mesh : meshes) {
        const u32 transform_index = mesh->transform_index;

        const StaticMesh* static_mesh = mesh->component.mesh().get();
        if(!static_mesh || transform_index == u32(-1)) {
            continue;
        }

        const core::Span<AssetPtr<Material>> materials = mesh->component.materials();
        if(materials.size() == 1) {
            if(const Material* mat = materials[0].get()) {
                y_debug_assert(!static_mesh->draw_command().vk_indirect_data().vertexOffset);
                batches.emplace_back(
                    mat->material_template(),
                    static_mesh->draw_command().vk_indirect_data(),
                    shader::MeshObject{transform_index, mat->draw_data().index(), static_mesh->mesh_data_index()}
                );
            }
        } else {
            y_debug_assert(static_mesh->sub_meshes().size() == materials.size());
            for(usize i = 0; i != materials.size(); ++i) {
                if(const Material* mat = materials[i].get()) {
                    batches.emplace_back(
                        mat->material_template(),
                        static_mesh->sub_meshes()[i].vk_indirect_data(),
                        shader::MeshObject{transform_index, mat->draw_data().index(), static_mesh->mesh_data_index()}
                    );
                }
            }
        }
    }

    {
        y_profile_zone("sort batches");
        std::sort(batches.begin(), batches.end(), [](const StaticMeshBatch& a, const StaticMeshBatch& b) { return a.material_template < b.material_template; });
    }
}

static void collect_batches_for_id(core::Span<const StaticMeshObject*> meshes, core::Vector<StaticMeshBatch>& batches) {
    y_profile();

    batches.set_min_capacity(meshes.size());

    u32 index = 0;
    for(const StaticMeshObject* mesh : meshes) {
        const u32 transform_index = mesh->transform_index;

        const StaticMesh* static_mesh = mesh->component.mesh().get();
        if(!static_mesh || transform_index == u32(-1)) {
            continue;
        }

        batches.emplace_back(
            nullptr,
            static_mesh->draw_command().vk_indirect_data(index),
            shader::MeshObject{transform_index, mesh->entity_index, static_mesh->mesh_data_index()}
        );

        ++index;
    }
}

Scene::RenderFunc Scene::prepare_render(FrameGraphPassBuilder& builder, i32 desc_set_index, const SceneVisibility& visibility, PassType pass_type) const {
    y_profile();


    // This is needed because std::function requires the lambda to be coyable
    // Might be fixed by std::move_only_function in C++23
    Y_TODO(fix in cpp23)
    auto static_mesh_batches = std::make_shared<core::Vector<StaticMeshBatch>>();
    {
        switch(pass_type) {
            case PassType::Depth:
            case PassType::GBuffer:
                Y_TODO(use visibility pass instead)
                collect_batches(visibility.meshes, *static_mesh_batches);
            break;

            case PassType::Id:
                collect_batches_for_id(visibility.meshes, *static_mesh_batches);
            break;
        }
    }


    const usize batch_count = static_mesh_batches->size();
    if(!batch_count) {
        return {};
    }



    const auto object_buffer = builder.declare_typed_buffer<shader::MeshObject>(batch_count);
    builder.map_buffer(object_buffer);

    const auto indirect_buffer = builder.declare_typed_buffer<VkDrawIndexedIndirectCommand>(batch_count);
    builder.map_buffer(indirect_buffer);

    builder.add_external_input(Descriptor(_transform_manager.transform_buffer()), PipelineStage::None, desc_set_index);
    builder.add_external_input(Descriptor(material_allocator().material_buffer()), PipelineStage::None, desc_set_index);
    builder.add_external_input(Descriptor(mesh_allocator().mesh_data_buffer()), PipelineStage::None, desc_set_index);
    builder.add_storage_input(object_buffer, PipelineStage::None, desc_set_index);

    builder.add_indrect_input(indirect_buffer);


    return [=](RenderPassRecorder& render_pass, const FrameGraphPass* pass) {
        y_profile_zone("scene render");

        y_debug_assert(!static_mesh_batches->is_empty());

        const core::Span<StaticMeshBatch> batches = *static_mesh_batches;

        const IndirectSubBuffer buffer = pass->resources().buffer<BufferUsage::IndirectBit>(indirect_buffer);

        auto indirect_mapping = pass->resources().map_buffer(indirect_buffer);
        auto object_mapping = pass->resources().map_buffer(object_buffer);

        render_pass.bind_index_buffer(mesh_allocator().triangle_buffer());

        switch(pass_type) {
            case PassType::Depth:
            case PassType::GBuffer: {
                const std::array<DescriptorSetProxy, 2> desc_sets = {
                    pass->descriptor_set(desc_set_index),
                    texture_library().descriptor_set()
                };

                usize start_of_batch = 0;
                const MaterialTemplate* prev_template = batches[0].material_template;
                auto push_batch = [&](const MaterialTemplate* material_template, usize index) {
                    if(prev_template == material_template) {
                        return;
                    }

                    if(const usize batch_size = index - start_of_batch) {
                        if(prev_template) {
                            render_pass.bind_material_template(prev_template, desc_sets);
                            render_pass.draw_indirect(IndirectSubBuffer(buffer, batch_size, start_of_batch));
                        }

                        start_of_batch = index;
                    }

                    prev_template = material_template;
                };

                for(usize i = 0; i != batches.size(); ++i) {
                    StaticMeshBatch batch = batches[i];
                    batch.cmd.firstInstance = u32(i);

                    indirect_mapping[i] = batch.cmd;
                    object_mapping[i] = batch.mesh_object;

                    push_batch(batch.material_template, i);
                }
                push_batch(nullptr, batches.size());
            } break;

            case PassType::Id: {
                for(usize i = 0; i != batches.size(); ++i) {
                    const StaticMeshBatch& batch = batches[i];
                    indirect_mapping[i] = batch.cmd;
                    object_mapping[i] = batch.mesh_object;
                }
                y_debug_assert(buffer.size() == batch_count);

                render_pass.bind_material_template(device_resources()[DeviceResources::IdMaterialTemplate], pass->descriptor_set(desc_set_index));
                render_pass.draw_indirect(buffer);
            } break;
        }
    };
}

}

