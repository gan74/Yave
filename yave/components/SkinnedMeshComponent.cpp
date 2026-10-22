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

#include "SkinnedMeshComponent.h"

#include <yave/meshes/MeshData.h>
#include <yave/graphics/images/ImageData.h>

#include <yave/assets/AssetLoader.h>

#include <yave/ecs/ComponentInspector.h>

namespace yave {

SkinnedMeshComponent::SkinnedMeshComponent(const AssetPtr<StaticMesh>& mesh, core::Vector<AssetPtr<Material>> materials, const AssetPtr<Animation>& animation) :
        _mesh(mesh), _materials(std::move(materials)), _animation(animation) {
}

SkinnedMeshComponent::SkinnedMeshComponent(const SkinnedMeshComponent& other) :
        _mesh(other._mesh), _materials(other._materials), _animation(other._animation) {
}

SkinnedMeshComponent& SkinnedMeshComponent::operator=(const SkinnedMeshComponent& other) {
    if(this != &other) {
        _mesh = other._mesh;
        _materials = other._materials;
        _animation = other._animation;
        _skeleton_instance = nullptr;
    }
    return *this;
}

AssetPtr<StaticMesh>& SkinnedMeshComponent::mesh() {
    return _mesh;
}

const AssetPtr<StaticMesh>& SkinnedMeshComponent::mesh() const {
    return _mesh;
}

core::MutableSpan<AssetPtr<Material>> SkinnedMeshComponent::materials() {
    return _materials;
}

core::Span<AssetPtr<Material>> SkinnedMeshComponent::materials() const {
    return _materials;
}

AssetPtr<Animation>& SkinnedMeshComponent::animation() {
    return _animation;
}

const AssetPtr<Animation>& SkinnedMeshComponent::animation() const {
    return _animation;
}

SkeletonInstance* SkinnedMeshComponent::skeleton_instance() {
    return _skeleton_instance.get();
}

const SkeletonInstance* SkinnedMeshComponent::skeleton_instance() const {
    return _skeleton_instance.get();
}

AABB SkinnedMeshComponent::aabb() const {
    return _mesh ? _mesh->aabb() : AABB();
}

bool SkinnedMeshComponent::is_fully_loaded() const {
    if(_mesh.is_loading() || _animation.is_loading()) {
        return false;
    }

    for(const auto& mat : _materials) {
        if(mat.is_loading()) {
            return false;
        }
    }

    return true;
}

bool SkinnedMeshComponent::update_asset_loading_status() {
    if(!is_fully_loaded()) {
        return false;
    }

    _skeleton_instance = nullptr;
    if(_mesh && _mesh->skeleton()) {
        _skeleton_instance = std::make_unique<SkeletonInstance>(_mesh->skeleton());
        _skeleton_instance->animate(_animation);
    }

    return true;
}

void SkinnedMeshComponent::load_assets(AssetLoadingContext& loading_ctx) {
    _mesh.load_async(loading_ctx);
    _animation.load_async(loading_ctx);

    for(auto& material : _materials) {
        material.load_async(loading_ctx);
    }
}

void SkinnedMeshComponent::inspect(ecs::ComponentInspector* inspector) {
    inspector->inspect("Mesh", _mesh);
    inspector->inspect("Materials", core::MutableSpan<AssetPtr<Material>>(_materials));
    inspector->inspect("Animation", _animation);

    if(_mesh) {
        const usize slots = _mesh->sub_meshes().size();
        if(slots != _materials.size()) {
            _materials = core::Vector<AssetPtr<Material>>(slots, AssetPtr<Material>());
        }
    }
}

}

