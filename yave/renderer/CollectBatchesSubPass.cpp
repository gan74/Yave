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

#include "CollectBatchesSubPass.h"

#include <yave/material/MaterialTemplate.h>
#include <yave/graphics/device/DeviceResources.h>

namespace yave {

template<typename F>
static void collect_batches(core::Span<const StaticMeshObject*> meshes, core::Vector<StaticMeshBatch>& batches, PassType pass_type, F&& mat_filter) {
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
                const MaterialTemplate* templ = mat->material_template(pass_type);
                if(!templ || !mat_filter(*mat)) {
                    continue;
                }
                batches.emplace_back(
                    templ,
                    static_mesh->draw_command().vk_indirect_data(),
                    shader::MeshObject{transform_index, mat->draw_data().index(), static_mesh->mesh_data_index()}
                );
            }
        } else {
            y_debug_assert(static_mesh->sub_meshes().size() == materials.size());
            for(usize i = 0; i != materials.size(); ++i) {
                if(const Material* mat = materials[i].get()) {
                    const MaterialTemplate* templ = mat->material_template(pass_type);
                    if(!templ || !mat_filter(*mat)) {
                        continue;
                    }
                    batches.emplace_back(
                        templ,
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

CollectBatchesSubPass CollectBatchesSubPass::create(const SceneVisibilitySubPass& visibility, PassType pass_type) {
    CollectBatchesSubPass pass;
    pass.pass_type = pass_type;
    pass.batches = std::make_shared<SceneBatches>();

    switch(pass_type) {
        case PassType::Depth:
            collect_batches(visibility.visible->meshes, pass.batches->static_mesh_batches, pass_type, [=](const Material&) { return true; });
        break;

        case PassType::GBuffer:
            collect_batches(visibility.visible->meshes, pass.batches->static_mesh_batches, pass_type, [=](const Material& mat) { return !mat.is_transparent(); });
        break;

        case PassType::Forward:
            collect_batches(visibility.visible->meshes, pass.batches->static_mesh_batches, pass_type, [=](const Material& mat) { return mat.is_transparent(); });
        break;

        case PassType::Id:
            collect_batches_for_id(visibility.visible->meshes, pass.batches->static_mesh_batches);
        break;
    }

    y_profile_msg(fmt_c_str("Collected {} batches",  pass.batches->static_mesh_batches.size()));

    return pass;
}

}

