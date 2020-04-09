/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include <yave/scene/Renderable.h>

#include "TransformableComponent.h"

namespace yave {

class StaticMeshComponent final : public Renderable, public ecs::RequiredComponents<TransformableComponent> {

	public:
		StaticMeshComponent() = default;
		StaticMeshComponent(const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& material);

		//StaticMeshComponent(StaticMeshComponent&&) = default;
		//StaticMeshComponent& operator=(StaticMeshComponent&&) = default;
		//~StaticMeshComponent();

		void flush_reload();

		void render(RenderPassRecorder& recorder, const SceneData& scene_data) const;
		void render_mesh(RenderPassRecorder& recorder, u32 instance_index) const;

		const AsyncAssetPtr<StaticMesh>& mesh() const;
		const AsyncAssetPtr<Material>& material() const;

		AsyncAssetPtr<StaticMesh>& mesh();
		AsyncAssetPtr<Material>& material();

		y_serde3(_async_mesh, _async_material)

	private:
		AsyncAssetPtr<StaticMesh> _async_mesh;
		AsyncAssetPtr<Material> _async_material;
};

}

#endif // YAVE_COMPONENTS_STATICMESHCOMPONENT_H
