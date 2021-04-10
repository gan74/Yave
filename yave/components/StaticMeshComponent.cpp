/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include "StaticMeshComponent.h"

#include <yave/meshes/StaticMesh.h>
#include <yave/material/Material.h>
#include <yave/material/Material.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/meshes/MeshData.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/assets/AssetLoader.h>

namespace yave {

StaticMeshComponent::SubMesh::SubMesh(const AssetPtr<StaticMesh>& me, const AssetPtr<Material>& ma) : mesh(me), material(ma) {
}

void StaticMeshComponent::SubMesh::render(RenderPassRecorder& recorder, const SceneData& scene_data) const {
    if(!material || !mesh) {
        return;
    }

    y_debug_assert(!material->is_null());
    y_debug_assert(!mesh->is_null());

    if(material->descriptor_set().is_null()) {
        recorder.bind_material(material->material_template(), {scene_data.descriptor_set});
    } else {
        recorder.bind_material(material->material_template(), {scene_data.descriptor_set, material->descriptor_set()});
    }

    render_mesh(recorder, scene_data.instance_index);
}

void StaticMeshComponent::SubMesh::render_mesh(RenderPassRecorder& recorder, u32 instance_index) const {
    if(!mesh) {
        return;
    }

    recorder.bind_buffers(TriangleSubBuffer(mesh->triangle_buffer()), VertexSubBuffer(mesh->vertex_buffer()));
    VkDrawIndexedIndirectCommand indirect = mesh->indirect_data();
    indirect.firstInstance = instance_index;
    recorder.draw(indirect);
}


bool StaticMeshComponent::SubMesh::operator==(const SubMesh& other) const {
    return mesh == other.mesh && material == other.material;
}

bool StaticMeshComponent::SubMesh::operator!=(const SubMesh& other) const {
    return !operator==(other);
}


StaticMeshComponent::StaticMeshComponent(const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& material) {
    _sub_meshes.emplace_back(mesh, material);
}

StaticMeshComponent::StaticMeshComponent(core::Vector<SubMesh> sub_meshes) : _sub_meshes(std::move(sub_meshes)) {
}

void StaticMeshComponent::render(RenderPassRecorder& recorder, const SceneData& scene_data) const {
    for(const SubMesh& sub : _sub_meshes) {
        sub.render(recorder, scene_data);
    }
}

void StaticMeshComponent::render_mesh(RenderPassRecorder& recorder, u32 instance_index) const {
    for(const SubMesh& sub : _sub_meshes) {
        sub.render_mesh(recorder, instance_index);
    }
}

const core::Vector<StaticMeshComponent::SubMesh>& StaticMeshComponent::sub_meshes() const {
    return _sub_meshes;
}

core::Vector<StaticMeshComponent::SubMesh>& StaticMeshComponent::sub_meshes() {
    return _sub_meshes;
}

const AABB& StaticMeshComponent::aabb() const {
    return _aabb;
}

AABB StaticMeshComponent::compute_aabb() const {
    AABB aabb;
    usize i = 0;
    for(i = 0; i != _sub_meshes.size(); ++i) {
        if(_sub_meshes[i].mesh) {
            aabb = _sub_meshes[i].mesh->aabb();
            break;
        }
    }

    for(i += 1; i < _sub_meshes.size(); ++i) {
        aabb = aabb.merged(_sub_meshes[i].mesh->aabb());
    }

    return aabb;
}

bool StaticMeshComponent::update_asset_loading_status() {
    for(const SubMesh& sub : _sub_meshes) {
       if(!sub.mesh.is_loaded() || !sub.material.is_loaded()) {
           return false;
       }
    }

    _aabb = compute_aabb();
    return true;
}

void StaticMeshComponent::load_assets(AssetLoadingContext& loading_ctx) {
    for(auto& sub : _sub_meshes) {
        sub.mesh.load(loading_ctx);
        sub.material.load(loading_ctx);
    }
}

}

