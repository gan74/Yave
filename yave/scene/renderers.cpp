/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

#include "renderers.h"
#include "Scene.h"
#include "SceneVisibility.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/device/MeshAllocator.h>
#include <yave/graphics/device/MaterialAllocator.h>
#include <yave/graphics/images/TextureLibrary.h>

#include <yave/material/MaterialTemplate.h>

namespace yave {

struct MeshBatch {
    const MaterialTemplate* material_template = nullptr;
    VkDrawIndexedIndirectCommand cmd = {};
    shader::MeshObject mesh_object;
};

static bool accept_material(const Material& mat, PassType pass_type) {
    switch(pass_type) {
        case PassType::GBuffer:
            return !mat.is_transparent();

        case PassType::Forward:
            return mat.is_transparent();

        default:
            return true;
    }
}

template<typename O, typename F>
static core::Vector<MeshBatch> collect_batches(core::Span<const O*> meshes, PassType pass_type, F&& mesh_data_index) {
    y_profile();

    auto batches = core::Vector<MeshBatch>::with_capacity(pass_type == PassType::Id ? meshes.size() : meshes.size() * 4);
    for(const O* mesh : meshes) {
        const u32 transform_index = mesh->transform_index;

        const StaticMesh* static_mesh = mesh->component.mesh().get();
        if(!static_mesh || transform_index == u32(-1)) {
            continue;
        }

        const u32 data_index = mesh_data_index(*mesh);
        if(data_index == u32(-1)) {
            continue;
        }

        if(pass_type == PassType::Id) {
            batches.emplace_back(
                nullptr,
                static_mesh->draw_command().vk_indirect_data(),
                shader::MeshObject{transform_index, mesh->entity_index, data_index}
            );
            continue;
        }

        const core::Span<AssetPtr<Material>> materials = mesh->component.materials();
        const bool single_material = materials.size() == 1;
        y_debug_assert(single_material ? !static_mesh->draw_command().vk_indirect_data().vertexOffset : static_mesh->sub_meshes().size() == materials.size());

        for(usize i = 0; i != materials.size(); ++i) {
            if(const Material* mat = materials[i].get()) {
                const MaterialTemplate* templ = mat->material_template(pass_type);
                if(!templ || !accept_material(*mat, pass_type)) {
                    continue;
                }
                batches.emplace_back(
                    templ,
                    single_material ? static_mesh->draw_command().vk_indirect_data() : static_mesh->sub_meshes()[i].vk_indirect_data(),
                    shader::MeshObject{transform_index, mat->draw_data().index(), data_index}
                );
            }
        }
    }

    if(pass_type != PassType::Id) {
        y_profile_zone("sort batches");
        std::sort(batches.begin(), batches.end(), [](const MeshBatch& a, const MeshBatch& b) { return a.material_template < b.material_template; });
    }

    y_profile_msg(fmt_c_str("Collected {} batches", batches.size()));

    return batches;
}

static RenderFunc prepare_batches(FrameGraphPassBuilder& builder, const RenderContext& ctx, core::Vector<MeshBatch> mesh_batches, SubBuffer<BufferUsage::StorageBit> mesh_datas) {
    if(mesh_batches.is_empty()) {
        return {};
    }

    const auto batches = std::make_shared<core::Vector<MeshBatch>>(std::move(mesh_batches));

    const auto object_buffer = builder.declare_typed_buffer<shader::MeshObject>(batches->size());
    builder.map_buffer(object_buffer);

    const auto indirect_buffer = builder.declare_typed_buffer<VkDrawIndexedIndirectCommand>(batches->size());
    builder.map_buffer(indirect_buffer);

    const i32 desc_set_index = ctx.descriptor_set_index;
    builder.add_uniform_input(ctx.camera, PipelineStage::None, desc_set_index);
    builder.add_external_input(Descriptor(ctx.scene->transform_manager().transform_buffer()), PipelineStage::None, desc_set_index);
    builder.add_external_input(Descriptor(material_allocator().material_buffer()), PipelineStage::None, desc_set_index);
    builder.add_external_input(Descriptor(mesh_datas), PipelineStage::None, desc_set_index);
    builder.add_storage_input(object_buffer, PipelineStage::None, desc_set_index);

    builder.add_indrect_input(indirect_buffer);

    const PassType pass_type = ctx.pass_type;

    return [=](RenderPassRecorder& render_pass, const FrameGraphPass* pass) {
        y_profile_zone("mesh render");

        const IndirectSubBuffer buffer = pass->resources().buffer<BufferUsage::IndirectBit>(indirect_buffer);

        {
            auto indirect_mapping = pass->resources().map_buffer(indirect_buffer);
            auto object_mapping = pass->resources().map_buffer(object_buffer);

            for(usize i = 0; i != batches->size(); ++i) {
                MeshBatch batch = (*batches)[i];
                batch.cmd.firstInstance = u32(i);

                indirect_mapping[i] = batch.cmd;
                object_mapping[i] = batch.mesh_object;
            }
        }

        render_pass.bind_index_buffer(mesh_allocator().triangle_buffer());

        if(pass_type == PassType::Id) {
            render_pass.bind_material_template(device_resources()[DeviceResources::IdMaterialTemplate], pass->descriptor_set(desc_set_index));
            render_pass.draw_indirect(buffer);
            return;
        }

        const std::array<DescriptorSetProxy, 2> desc_sets = {
            pass->descriptor_set(desc_set_index),
            texture_library().descriptor_set()
        };

        usize start_of_batch = 0;
        for(usize i = 1; i <= batches->size(); ++i) {
            const MaterialTemplate* material_template = (*batches)[start_of_batch].material_template;
            if(i == batches->size() || (*batches)[i].material_template != material_template) {
                render_pass.bind_material_template(material_template, desc_sets);
                render_pass.draw_indirect(IndirectSubBuffer(buffer, i - start_of_batch, start_of_batch));
                start_of_batch = i;
            }
        }
    };
}





RenderFunc StaticMeshRenderer::prepare(FrameGraphPassBuilder& builder, const RenderContext& ctx, const SceneVisibility& visible) const {
    y_profile();

    auto batches = collect_batches(core::Span<const StaticMeshObject*>(visible.meshes), ctx.pass_type, [](const StaticMeshObject& obj) {
        return obj.component.mesh()->mesh_data_index();
    });

    return prepare_batches(builder, ctx, std::move(batches), mesh_allocator().mesh_data_buffer());
}




void SkinnedMeshRenderer::update(ComputeCapableCmdBufferRecorder& recorder, Scene& scene) {
    y_profile();

    const core::MutableSpan<SkinnedMeshObject> objects = scene._skinned_meshes;

    if(objects.is_empty()) {
        _mesh_datas = {};
        return;
    }

    const auto region = recorder.region("Skinning");
    const auto& program = device_resources()[DeviceResources::SkinningProgram];

    MeshDataBuffer mesh_datas(objects.size());
    {
        auto mapping = mesh_datas.map(MappingAccess::WriteOnly);

        for(usize i = 0; i != objects.size(); ++i) {
            SkinnedMeshObject& obj = objects[i];
            obj.mesh_data_index = u32(-1);
            mapping[i] = {};

            const StaticMesh* mesh = obj.component.mesh().get();
            SkeletonInstance* instance = obj.component.skeleton_instance();
            const u32 vertex_count = mesh ? mesh->draw_data().vertex_count() : 0;
            if(!instance || !vertex_count) {
                obj.skinned_positions = {};
                continue;
            }

            const u64 byte_size = vertex_count * sizeof(math::Vec3);
            if(obj.skinned_positions.byte_size() != byte_size) {
                obj.skinned_positions = PositionBuffer(byte_size);
            }

            instance->update();

            const MeshDrawBuffers buffers = mesh->draw_data().mesh_buffers();

            const auto descriptors = make_descriptor_set(
                obj.skinned_positions,
                SubBuffer<BufferUsage::StorageBit>(buffers.attribs[usize(VertexStreamType::Position)]),
                mesh->skin_weights(),
                instance->bone_transform_buffer(),
                InlineDescriptor(vertex_count)
            );

            recorder.dispatch_threads(program, math::Vec2ui(vertex_count, 1), DescriptorSetProxy(descriptors));

            mapping[i] = shader::StaticMeshData {
                obj.skinned_positions.vk_device_address(),
                buffers.attribs[usize(VertexStreamType::NormalTangent)].vk_device_address(),
                buffers.attribs[usize(VertexStreamType::Uv)].vk_device_address(),
                0
            };
            obj.mesh_data_index = u32(i);
        }
    }

    _mesh_datas = std::move(mesh_datas);
}

RenderFunc SkinnedMeshRenderer::prepare(FrameGraphPassBuilder& builder, const RenderContext& ctx, const SceneVisibility& visible) const {
    y_profile();

    auto batches = collect_batches(core::Span<const SkinnedMeshObject*>(visible.skinned_meshes), ctx.pass_type, [](const SkinnedMeshObject& obj) {
        return obj.mesh_data_index;
    });

    return prepare_batches(builder, ctx, std::move(batches), _mesh_datas);
}

}

