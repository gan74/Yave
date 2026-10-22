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

static SceneRenderSubPass::RenderFunc prepare_scene_render(SceneRenderSubPass& scene_pass, FrameGraphPassBuilder& builder, PassType pass_type) {
    y_profile();

    const Scene* scene = scene_pass.scene_view.scene();

    const std::shared_ptr scene_batches = scene_pass.batches.batches;
    const usize static_count = scene_batches->static_mesh_batches.size();
    const usize skinned_count = scene_batches->skinned_mesh_batches.size();
    const usize batch_count = static_count + skinned_count;

    scene_pass.descriptor_set_index = builder.next_descriptor_set_index();
    builder.add_uniform_input(scene_pass.camera, PipelineStage::None, scene_pass.descriptor_set_index);

    if(!batch_count) {
        return {};
    }

    const auto object_buffer = builder.declare_typed_buffer<shader::MeshObject>(batch_count);
    builder.map_buffer(object_buffer);

    const auto indirect_buffer = builder.declare_typed_buffer<VkDrawIndexedIndirectCommand>(batch_count);
    builder.map_buffer(indirect_buffer);

    auto add_scene_bindings = [&](i32 desc_set_index, SubBuffer<BufferUsage::StorageBit> mesh_datas) {
        builder.add_external_input(Descriptor(scene->transform_manager().transform_buffer()), PipelineStage::None, desc_set_index);
        builder.add_external_input(Descriptor(material_allocator().material_buffer()), PipelineStage::None, desc_set_index);
        builder.add_external_input(Descriptor(mesh_datas), PipelineStage::None, desc_set_index);
        builder.add_storage_input(object_buffer, PipelineStage::None, desc_set_index);
    };

    add_scene_bindings(scene_pass.descriptor_set_index, mesh_allocator().mesh_data_buffer());

    if(skinned_count) {
        scene_pass.skinned_descriptor_set_index = builder.next_descriptor_set_index();
        builder.add_uniform_input(scene_pass.camera, PipelineStage::None, scene_pass.skinned_descriptor_set_index);
        add_scene_bindings(scene_pass.skinned_descriptor_set_index, scene->skinned_mesh_renderer().mesh_data_buffer());
    }

    const i32 static_set_index = scene_pass.descriptor_set_index;
    const i32 skinned_set_index = scene_pass.skinned_descriptor_set_index;

    builder.add_indrect_input(indirect_buffer);

    return [=](RenderPassRecorder& render_pass, const FrameGraphPass* pass) {
        y_profile_zone("scene render");

        const IndirectSubBuffer buffer = pass->resources().buffer<BufferUsage::IndirectBit>(indirect_buffer);

        auto indirect_mapping = pass->resources().map_buffer(indirect_buffer);
        auto object_mapping = pass->resources().map_buffer(object_buffer);

        render_pass.bind_index_buffer(mesh_allocator().triangle_buffer());

        auto render_batches = [&](core::Span<StaticMeshBatch> batches, usize offset, i32 desc_set_index) {
            if(batches.is_empty()) {
                return;
            }

            for(usize i = 0; i != batches.size(); ++i) {
                StaticMeshBatch batch = batches[i];
                batch.cmd.firstInstance = u32(offset + i);

                indirect_mapping[offset + i] = batch.cmd;
                object_mapping[offset + i] = batch.mesh_object;
            }

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
                                render_pass.draw_indirect(IndirectSubBuffer(buffer, batch_size, offset + start_of_batch));
                            }

                            start_of_batch = index;
                        }

                        prev_template = material_template;
                    };

                    for(usize i = 0; i != batches.size(); ++i) {
                        push_batch(batches[i].material_template, i);
                    }
                    push_batch(nullptr, batches.size());
                } break;

                case PassType::Id: {
                    render_pass.bind_material_template(device_resources()[DeviceResources::IdMaterialTemplate], pass->descriptor_set(desc_set_index));
                    render_pass.draw_indirect(IndirectSubBuffer(buffer, batches.size(), offset));
                } break;
            }
        };

        render_batches(scene_batches->static_mesh_batches, 0, static_set_index);
        render_batches(scene_batches->skinned_mesh_batches, static_count, skinned_set_index);
    };
}




static void fill_scene_render_pass(SceneRenderSubPass& pass, FrameGraphPassBuilder& builder, PassType pass_type) {
    pass.batches = CollectBatchesSubPass::create(pass.visibility, pass_type);
    pass.render_func = prepare_scene_render(pass, builder, pass_type);
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

