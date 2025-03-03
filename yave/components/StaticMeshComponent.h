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
#ifndef YAVE_COMPONENTS_STATICMESHCOMPONENT_H
#define YAVE_COMPONENTS_STATICMESHCOMPONENT_H

#include <yave/ecs/ecs.h>

#include <yave/meshes/StaticMesh.h>
#include <yave/material/Material.h>

#include <yave/systems/AssetLoaderSystem.h>

#include <y/core/Vector.h>

namespace yave {

class StaticMeshComponent final :
    public ecs::RegisterComponent<StaticMeshComponent, AssetLoaderSystem>,
    public ecs::RequireComponent<TransformableComponent> {

    public:
        StaticMeshComponent() = default;
        StaticMeshComponent(const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& material);
        StaticMeshComponent(const AssetPtr<StaticMesh>& mesh, core::Vector<AssetPtr<Material>> materials);

        AssetPtr<StaticMesh>& mesh();
        const AssetPtr<StaticMesh>& mesh() const;

        const BLAS* blas() const;

        core::MutableSpan<AssetPtr<Material>> materials();
        core::Span<AssetPtr<Material>> materials() const;

        AABB aabb() const;

        bool is_fully_loaded() const;

        bool update_asset_loading_status();
        void load_assets(AssetLoadingContext& loading_ctx);

        void inspect(ecs::ComponentInspector* inspector);

        y_reflect(StaticMeshComponent, _mesh, _materials)

    private:
        AssetPtr<StaticMesh> _mesh;
        core::Vector<AssetPtr<Material>> _materials;
};


static_assert(ecs::HasRequiredComponents<StaticMeshComponent>);
static_assert(ecs::Registerable<StaticMeshComponent>);
static_assert(ecs::Inspectable<StaticMeshComponent>);

}

#endif // YAVE_COMPONENTS_STATICMESHCOMPONENT_H

