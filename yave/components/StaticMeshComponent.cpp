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

#include "StaticMeshComponent.h"

#include <yave/scene/Scene.h>

#include <yave/meshes/MeshData.h>
#include <yave/graphics/images/ImageData.h>

#include <yave/assets/AssetLoader.h>

#include <yave/ecs/ComponentInspector.h>

namespace yave {

StaticMeshComponent::StaticMeshComponent(const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& material) :
        _mesh(mesh) {

    _materials << material;
}

StaticMeshComponent::StaticMeshComponent(const AssetPtr<StaticMesh>& mesh, core::Vector<AssetPtr<Material>> materials) :
        _mesh(mesh), _materials(std::move(materials)) {
}

AssetPtr<StaticMesh>& StaticMeshComponent::mesh() {
    return _mesh;
}

const AssetPtr<StaticMesh>& StaticMeshComponent::mesh() const {
    return _mesh;
}

core::MutableSpan<AssetPtr<Material>> StaticMeshComponent::materials() {
    return _materials;
}

core::Span<AssetPtr<Material>> StaticMeshComponent::materials() const {
    return _materials;
}

AABB StaticMeshComponent::aabb() const {
    return _mesh ? _mesh->aabb() : AABB();
}

bool StaticMeshComponent::is_fully_loaded() const {
    if(_mesh.is_loading()) {
        return false;
    }

    for(const auto& mat : _materials) {
        if(mat.is_loading()) {
            return false;
        }
    }

    return true;
}

bool StaticMeshComponent::update_asset_loading_status() {
    return is_fully_loaded();
}

void StaticMeshComponent::load_assets(AssetLoadingContext& loading_ctx) {
    _mesh.load_async(loading_ctx);

    for(auto& material : _materials) {
        material.load_async(loading_ctx);
    }
}

void StaticMeshComponent::inspect(ecs::ComponentInspector* inspector) {
    inspector->inspect("Mesh", _mesh);
    inspector->inspect("Materials", core::MutableSpan<AssetPtr<Material>>(_materials));

    if(_mesh) {
        const usize slots = _mesh->sub_meshes().size();
        if(slots != _materials.size()) {
            _materials = core::Vector<AssetPtr<Material>>(slots, AssetPtr<Material>());
        }
    }
}

}

