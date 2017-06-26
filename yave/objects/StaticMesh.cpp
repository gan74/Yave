/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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

#include "StaticMesh.h"

#include <yave/material/Material.h>

namespace yave {


StaticMesh::StaticMesh(const AssetPtr<StaticMeshInstance>& instance, const AssetPtr<Material>& material) :
		_instance(instance),
		_material(material) {

	set_radius(_instance->radius());
}

StaticMesh::StaticMesh(StaticMesh&& other) :
		Renderable(other),
		_instance(std::move(other._instance)),
		_material(std::move(other._material)) {
}

void StaticMesh::render(const FrameToken&, CmdBufferRecorderBase& recorder, const SceneData& scene_data) const {
	recorder.bind_material(*_material, {scene_data.descriptor_set});
	recorder.bind_buffers(TriangleSubBuffer(_instance->triangle_buffer()), {VertexSubBuffer(_instance->vertex_buffer()), scene_data.instance_attribs});
	recorder.draw(_instance->indirect_data());
}

}
