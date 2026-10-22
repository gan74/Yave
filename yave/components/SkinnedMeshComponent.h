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
#ifndef YAVE_COMPONENTS_SKINNEDMESHCOMPONENT_H
#define YAVE_COMPONENTS_SKINNEDMESHCOMPONENT_H

#include <yave/ecs/ecs.h>

#include <yave/meshes/StaticMesh.h>
#include <yave/material/Material.h>
#include <yave/animations/SkeletonInstance.h>

#include <yave/systems/AssetLoaderSystem.h>

#include <y/core/Vector.h>

#include <memory>

namespace yave {

class SkinnedMeshComponent final :
    public ecs::RegisterComponent<SkinnedMeshComponent, AssetLoaderSystem>,
    public ecs::RequireComponent<TransformableComponent> {

    public:
        SkinnedMeshComponent() = default;
        SkinnedMeshComponent(const AssetPtr<StaticMesh>& mesh, core::Vector<AssetPtr<Material>> materials, const AssetPtr<Animation>& animation = {});

        SkinnedMeshComponent(const SkinnedMeshComponent& other);
        SkinnedMeshComponent& operator=(const SkinnedMeshComponent& other);

        SkinnedMeshComponent(SkinnedMeshComponent&&) = default;
        SkinnedMeshComponent& operator=(SkinnedMeshComponent&&) = default;

        AssetPtr<StaticMesh>& mesh();
        const AssetPtr<StaticMesh>& mesh() const;

        core::MutableSpan<AssetPtr<Material>> materials();
        core::Span<AssetPtr<Material>> materials() const;

        AssetPtr<Animation>& animation();
        const AssetPtr<Animation>& animation() const;

        SkeletonInstance* skeleton_instance();
        const SkeletonInstance* skeleton_instance() const;

        AABB aabb() const;

        bool is_fully_loaded() const;

        bool update_asset_loading_status();
        void load_assets(AssetLoadingContext& loading_ctx);

        void inspect(ecs::ComponentInspector* inspector);

        y_reflect(SkinnedMeshComponent, _mesh, _materials, _animation)

    private:
        AssetPtr<StaticMesh> _mesh;
        core::Vector<AssetPtr<Material>> _materials;
        AssetPtr<Animation> _animation;

        std::unique_ptr<SkeletonInstance> _skeleton_instance;
};


static_assert(ecs::HasRequiredComponents<SkinnedMeshComponent>);
static_assert(ecs::Inspectable<SkinnedMeshComponent>);

}

#endif // YAVE_COMPONENTS_SKINNEDMESHCOMPONENT_H

