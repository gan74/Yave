/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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

#include "SceneRenderer.h"


namespace yave {

static constexpr usize batch_size = 128 * 1024;

SceneRenderer::SceneRenderer(DevicePtr dptr, const Node::Ptr<CullingNode>& cull):
		_cull(cull),

		_camera_buffer(dptr, 1),
		_camera_mapping(_camera_buffer.map()),
		_camera_set(dptr, {Binding(_camera_buffer)}),

		_matrix_buffer(dptr, batch_size),
		_matrix_mapping(_matrix_buffer.map()),

		_indirect_buffer(dptr, batch_size),
		_indirect_mapping(_indirect_buffer.map()) {

	log_msg("Allocating "_s + (batch_size / 1024) * (sizeof(math::Matrix4<>) + sizeof(vk::DrawIndexedIndirectCommand)) + "KB of scene buffers");
}

const SceneView& SceneRenderer::scene_view() const {
	return _cull->scene_view();
}

core::Vector<Node::NodePtr> SceneRenderer::dependencies() {
	return {_cull};
}

void SceneRenderer::process(const FrameToken&, CmdBufferRecorder<>& recorder) {
	_camera_mapping[0] = scene_view().camera().viewproj_matrix();

	usize i = 0;
	for(const auto& mesh : _cull->visibles()) {
		_matrix_mapping[i] = mesh->transform();
		auto i_data = mesh->instance().indirect_data;
		i_data.setFirstInstance(i);
		_indirect_mapping[i++] = i_data;

		if(i == batch_size) {
			break;
		}
	}

	if(!i) {
		return;
	}

	recorder.bind_pipeline(const_cast<Material&>(_cull->visibles().first()->material()).compile(recorder.current_pass(), recorder.viewport()), {_camera_set});

	vk::Buffer vertex_buffer = _cull->visibles().first()->instance().vertex_buffer.vk_buffer();
	vk::Buffer index_buffer = _cull->visibles().first()->instance().triangle_buffer.vk_buffer();

	recorder.vk_cmd_buffer().bindVertexBuffers(0, {vertex_buffer}, {0});
	recorder.vk_cmd_buffer().bindVertexBuffers(1, {_matrix_buffer.vk_buffer()}, {0});
	recorder.vk_cmd_buffer().bindIndexBuffer(index_buffer, 0, vk::IndexType::eUint32);

	recorder.vk_cmd_buffer().drawIndexedIndirect(_indirect_buffer.vk_buffer(), 0, i, sizeof(vk::DrawIndexedIndirectCommand));

	/*for(const auto& mesh : _cull->visibles()) {
		mesh->draw(recorder, _camera_set);
	}*/
}



}
