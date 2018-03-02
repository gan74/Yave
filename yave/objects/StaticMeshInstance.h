/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#ifndef YAVE_OBJECTS_STATICMESHINSTANCE_H
#define YAVE_OBJECTS_STATICMESHINSTANCE_H

#include <yave/assets/AssetPtr.h>
#include <yave/meshes/StaticMesh.h>

#include "Renderable.h"

namespace yave {

class StaticMeshInstance : public Renderable {

	public:
		StaticMeshInstance(const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& material);

		StaticMeshInstance(StaticMeshInstance&& other);
		StaticMeshInstance& operator=(StaticMeshInstance&& other) = delete;

		void render(const FrameToken&, CmdBufferRecorderBase& recorder, const SceneData& scene_data) const override;


		const auto& material() const {
			return _material;
		}

	private:
		AssetPtr<StaticMesh> _mesh;
		mutable AssetPtr<Material> _material;
};

}

#endif // YAVE_OBJECTS_STATICMESHINSTANCE_H
