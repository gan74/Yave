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

#include "StaticMeshInstance.h"

#include <yave/material/Material.h>

namespace yave {

StaticMeshInstance::StaticMeshInstance(const AssetPtr<StaticMesh>& mesh, const AssetPtr<Material>& material) :
		_mesh(mesh),
		_material(material) {

	set_radius(_mesh->radius());
}

StaticMeshInstance::StaticMeshInstance(StaticMeshInstance&& other) :
		Renderable(other),
		_mesh(std::move(other._mesh)),
		_material(std::move(other._material)) {
}

void StaticMeshInstance::render(RenderPassRecorder& recorder, const SceneData& scene_data) const {
	recorder.bind_material(*_material, {scene_data.descriptor_set});
	recorder.bind_buffers(TriangleSubBuffer(_mesh->triangle_buffer()), {VertexSubBuffer(_mesh->vertex_buffer())});

	auto indirect = _mesh->indirect_data();
	indirect.setFirstInstance(scene_data.instance_index);
	recorder.draw(indirect);
}

}
