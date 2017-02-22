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

#include <yave/commands/CmdBufferRecorder.h>
#include <yave/material/Material.h>
#include <yave/framebuffers/RenderPass.h>
#include <yave/device/Device.h>

namespace yave {


StaticMesh::StaticMesh(const AssetPtr<StaticMeshInstance>& instance, const AssetPtr<Material>& material) :
		_instance(instance),
		_material(material),
		_matrix_buffer(_material->device(), 1),
		_mapping(_matrix_buffer) {

	if(!instance) {
		fatal("Null instance.");
	}

	set_radius(_instance->radius);

	set_storage(_mapping.begin());
	transform() = math::identity();
}

StaticMesh::StaticMesh(StaticMesh&& other) :
		Transformable(other),
		_instance(std::move(other._instance)),
		_material(std::move(other._material)),
		_matrix_buffer(std::move(other._matrix_buffer)),
		_mapping(std::move(other._mapping)) {

	set_storage(_mapping.begin());
}

void StaticMesh::draw(CmdBufferRecorderBase& recorder, const DescriptorSet& vp) const {
	recorder.bind_pipeline(_material->compile(recorder.current_pass(), recorder.viewport()), {vp});
	recorder.vk_cmd_buffer().bindVertexBuffers(1, _matrix_buffer.vk_buffer(), vk::DeviceSize(0));
	recorder.draw(*_instance);
}

}
