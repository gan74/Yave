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

#include "SceneRenderSubPass.h"
#include "CameraBufferPass.h"

#include <yave/scene/EcsScene.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/device/MeshAllocator.h>
#include <yave/graphics/device/MaterialAllocator.h>
#include <yave/graphics/images/TextureLibrary.h>


#include <yave/ecs/EntityWorld.h>


namespace yave {

static SceneRenderSubPass::RenderFunc prepare_scene_render(const Scene* scene, FrameGraphPassBuilder& builder, i32 desc_set_index, const CollectBatchesSubPass& batches, PassType pass_type) {
    y_profile();

    const std::shared_ptr scene_batches = batches.batches;
    const usize batch_count = scene_batches->static_mesh_batches.size();
    if(!batch_count) {
        return {};
    }

    const auto object_buffer = builder.declare_typed_buffer<shader::MeshObject>(batch_count);
    builder.map_buffer(object_buffer);

    const auto indirect_buffer = builder.declare_typed_buffer<VkDrawIndexedIndirectCommand>(batch_count);
    builder.map_buffer(indirect_buffer);

    builder.add_external_input(Descriptor(scene->transform_manager().transform_buffer()), PipelineStage::None, desc_set_index);
    builder.add_external_input(Descriptor(material_allocator().material_buffer()), PipelineStage::None, desc_set_index);
    builder.add_external_input(Descriptor(mesh_allocator().mesh_data_buffer()), PipelineStage::None, desc_set_index);
    builder.add_storage_input(object_buffer, PipelineStage::None, desc_set_index);

    builder.add_indrect_input(indirect_buffer);

    return [=](RenderPassRecorder& render_pass, const FrameGraphPass* pass) {
        y_profile_zone("scene render");

        y_debug_assert(!scene_batches->static_mesh_batches.is_empty());

        const core::Span<StaticMeshBatch> batches = scene_batches->static_mesh_batches;

        const IndirectSubBuffer buffer = pass->resources().buffer<BufferUsage::IndirectBit>(indirect_buffer);

        auto indirect_mapping = pass->resources().map_buffer(indirect_buffer);
        auto object_mapping = pass->resources().map_buffer(object_buffer);

        render_pass.bind_index_buffer(mesh_allocator().triangle_buffer());

        switch(pass_type) {
            case PassType::Depth:
            case PassType::GBuffer:
            case PassType::Forward: {
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




static void fill_scene_render_pass(SceneRenderSubPass& pass, FrameGraphPassBuilder& builder, PassType pass_type) {
    pass.descriptor_set_index = builder.next_descriptor_set_index();
    builder.add_uniform_input(pass.camera, PipelineStage::None, pass.descriptor_set_index);

    pass.batches = CollectBatchesSubPass::create(pass.visibility, pass_type);
    pass.render_func = prepare_scene_render(pass.scene_view.scene(), builder, pass.descriptor_set_index, pass.batches, pass_type);
}


SceneRenderSubPass SceneRenderSubPass::create(FrameGraphPassBuilder& builder, const SceneView& scene_view, const SceneVisibilitySubPass& visibility, PassType pass_type) {
    const auto camera = builder.declare_typed_buffer<shader::Camera>();
    builder.map_buffer(camera, shader::Camera(scene_view.camera()));

    SceneRenderSubPass pass;
    pass.scene_view = scene_view;
    pass.camera = camera;
    pass.visibility = visibility;

    fill_scene_render_pass(pass, builder, pass_type);

    return pass;
}

SceneRenderSubPass SceneRenderSubPass::create(FrameGraphPassBuilder& builder, const CameraBufferPass& camera, const SceneVisibilitySubPass& visibility, PassType pass_type) {
    SceneRenderSubPass pass;
    pass.scene_view = camera.view;
    pass.camera = camera.camera;
    pass.visibility = visibility;

    fill_scene_render_pass(pass, builder, pass_type);

    return pass;
}

void SceneRenderSubPass::render(RenderPassRecorder& render_pass, const FrameGraphPass* pass) const {
    if(render_func) {
        render_func(render_pass, pass);
    }
}

}

