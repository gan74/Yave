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
#ifndef YAVE_COMPONENTS_STATICMESHCOMPONENT_H
#define YAVE_COMPONENTS_STATICMESHCOMPONENT_H

#include "HasLoadableAssetsTag.h"

#include "TransformableComponent.h"

#include <yave/scene/Renderable.h>
#include <yave/meshes/AABB.h>

#include <y/core/Vector.h>

namespace yave {

class StaticMeshComponent final :
        public Renderable,
        public ecs::RequiredComponents<TransformableComponent>,
        public HasLoadableAssetsTag<StaticMeshComponent> {

    public:
        struct SubMesh {
            AssetPtr<StaticMesh> mesh;
            AssetPtr<Material> material;

            SubMesh() = default;
            SubMesh(const AssetPtr<StaticMesh>& me, const AssetPtr<Material>& ma);

            void render(RenderPassRecorder& recorder, const SceneData& scene_data) const;
            void render_mesh(RenderPassRecorder& recorder, u32 instance_index) const;

            bool operator==(const SubMesh& other) const;
            bool operator!=(const SubMesh& other) const;

            y_reflect(mesh, material)
        };

        StaticMeshComponent() = default;
        StaticMeshComponent(const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& material);
        StaticMeshComponent(core::Vector<SubMesh> sub_meshes);

        void render(RenderPassRecorder& recorder, const SceneData& scene_data) const;
        void render_mesh(RenderPassRecorder& recorder, u32 instance_index) const;

        const core::Vector<SubMesh>& sub_meshes() const;
        core::Vector<SubMesh>& sub_meshes();

        const AABB& aabb() const;

        bool update_asset_loading_status();
        void load_assets(AssetLoadingContext& loading_ctx);

        y_reflect(_sub_meshes)

    private:
        AABB compute_aabb() const;

        core::Vector<SubMesh> _sub_meshes;
        AABB _aabb;

};

}

#endif // YAVE_COMPONENTS_STATICMESHCOMPONENT_H

