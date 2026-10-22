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

static u32 mesh_data_index(const Scene*, const StaticMeshObject* mesh) {
    return mesh->component.mesh()->mesh_data_index();
}

static u32 mesh_data_index(const Scene* scene, const SkinnedMeshObject* mesh) {
    return SkinnedMeshRenderer::is_ready(*mesh) ? u32(mesh - scene->skinned_meshes().data()) : u32(-1);
}

template<typename O, std::predicate<const Material&> F>
static void collect_batches(const Scene* scene, core::Span<const O*> meshes, core::Vector<StaticMeshBatch>& batches, PassType pass_type, F&& mat_filter) {
    y_profile();

    batches.set_min_capacity(meshes.size() * 4);
    for(const O* mesh : meshes) {
        const u32 transform_index = mesh->transform_index;

        const StaticMesh* static_mesh = mesh->component.mesh().get();
        if(!static_mesh || transform_index == u32(-1)) {
            continue;
        }

        const u32 data_index = mesh_data_index(scene, mesh);
        if(data_index == u32(-1)) {
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
                    shader::MeshObject{transform_index, mat->draw_data().index(), data_index}
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
                        shader::MeshObject{transform_index, mat->draw_data().index(), data_index}
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

template<typename O>
static void collect_batches_for_id(const Scene* scene, core::Span<const O*> meshes, core::Vector<StaticMeshBatch>& batches) {
    y_profile();

    batches.set_min_capacity(meshes.size());

    for(const O* mesh : meshes) {
        const u32 transform_index = mesh->transform_index;

        const StaticMesh* static_mesh = mesh->component.mesh().get();
        if(!static_mesh || transform_index == u32(-1)) {
            continue;
        }

        const u32 data_index = mesh_data_index(scene, mesh);
        if(data_index == u32(-1)) {
            continue;
        }

        batches.emplace_back(
            nullptr,
            static_mesh->draw_command().vk_indirect_data(),
            shader::MeshObject{transform_index, mesh->entity_index, data_index}
        );
    }
}

CollectBatchesSubPass CollectBatchesSubPass::create(const SceneVisibilitySubPass& visibility, PassType pass_type) {
    CollectBatchesSubPass pass;
    pass.pass_type = pass_type;
    pass.batches = std::make_shared<SceneBatches>();

    const Scene* scene = visibility.scene_view.scene();
    const SceneVisibility& visible = *visibility.visible;
    SceneBatches& batches = *pass.batches;

    auto collect = [&](auto&& mat_filter) {
        collect_batches(scene, core::Span<const StaticMeshObject*>(visible.meshes), batches.static_mesh_batches, pass_type, mat_filter);
        collect_batches(scene, core::Span<const SkinnedMeshObject*>(visible.skinned_meshes), batches.skinned_mesh_batches, pass_type, mat_filter);
    };

    switch(pass_type) {
        case PassType::Depth:
            collect([=](const Material&) { return true; });
        break;

        case PassType::GBuffer:
            collect([=](const Material& mat) { return !mat.is_transparent(); });
        break;

        case PassType::Forward:
            collect([=](const Material& mat) { return mat.is_transparent(); });
        break;

        case PassType::Id:
            collect_batches_for_id(scene, core::Span<const StaticMeshObject*>(visible.meshes), batches.static_mesh_batches);
            collect_batches_for_id(scene, core::Span<const SkinnedMeshObject*>(visible.skinned_meshes), batches.skinned_mesh_batches);
        break;
    }

    y_profile_msg(fmt_c_str("Collected {} batches",  pass.batches->static_mesh_batches.size()));

    return pass;
}

}

