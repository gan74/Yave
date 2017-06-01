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

vk::DrawIndexedIndirectCommand prepare_command(vk::DrawIndexedIndirectCommand cmd, usize i) {
	cmd.setFirstInstance(i);
	return cmd;
}

SceneRenderer::SceneRenderer(DevicePtr dptr, const Ptr<CullingNode>& cull) :
		DeviceLinked(dptr),
		_cull(cull),

		_camera_buffer(device(), 1),
		_camera_mapping(_camera_buffer.map()),
		_camera_set(device(), {Binding(_camera_buffer)}),

		_matrix_buffer(device(), batch_size),
		_matrix_mapping(_matrix_buffer.map()),

		_indirect_buffer(device(), batch_size),
		_indirect_mapping(_indirect_buffer.map()) {

	log_msg("Allocating "_s + (batch_size / 1024) * (sizeof(math::Matrix4<>) + sizeof(vk::DrawIndexedIndirectCommand)) + "KB of scene buffers");
}

const SceneView& SceneRenderer::scene_view() const {
	return _cull->scene_view();
}

const RecordedCmdBuffer<CmdBufferUsage::Secondary>& SceneRenderer::cmd_buffer() const {
	return _cmd_buffer;
}

void SceneRenderer::process(const FrameToken&, CmdBufferRecorder<CmdBufferUsage::Secondary>&& recorder) {
	_camera_mapping[0] = scene_view().camera().viewproj_matrix();

	const auto& visibles = _cull->visibles();

	if(visibles.is_empty()) {
		_cmd_buffer = recorder.end();
		return;
	}

	if(visibles.size() > batch_size) {
		log_msg("Visible object count exceeds scene buffer size", LogType::Warning);
	}

	usize i = 0;
	usize submitted = 0;
	AssetPtr<Material> current_material;
	std::array<vk::Buffer, 2> current_buffers;
	for(usize count = std::min(batch_size, visibles.size()); i != count; ++i) {
		const auto& mesh = visibles[i];

		const auto& instance = mesh->instance();
		const auto& material = mesh->material();

		std::array<vk::Buffer, 2> buffers = {{instance->vertex_buffer.vk_buffer(), instance->triangle_buffer.vk_buffer()}};
		if(buffers != current_buffers) {
			current_buffers = buffers;
			setup_instance(recorder, instance);
			submit_batches(recorder, current_material = material, submitted, i - submitted);
			submitted = i;
		} else if(current_material != material) {
			submit_batches(recorder, current_material = material, submitted, i - submitted);
			submitted = i;
		}

		_matrix_mapping[i] = mesh->transform();
		_indirect_mapping[i] = prepare_command(mesh->instance()->indirect_data, i);
	}
	submit_batches(recorder, visibles[submitted]->material(), submitted, i - submitted);

	_cmd_buffer = recorder.end();
}

void SceneRenderer::setup_instance(CmdBufferRecorder<CmdBufferUsage::Secondary>& recorder, const AssetPtr<StaticMeshInstance>& instance) {
	recorder.vk_cmd_buffer().bindVertexBuffers(0, {instance->vertex_buffer.vk_buffer(), _matrix_buffer.vk_buffer()}, {0, 0});
	recorder.vk_cmd_buffer().bindIndexBuffer(instance->triangle_buffer.vk_buffer(), 0, vk::IndexType::eUint32);
}

void SceneRenderer::submit_batches(CmdBufferRecorder<CmdBufferUsage::Secondary>& recorder, AssetPtr<Material>& mat, usize offset, usize size) {
	if(size) {
		recorder.bind_pipeline(mat->compile(recorder.current_pass()), {_camera_set});

		const usize cmd_size = sizeof(vk::DrawIndexedIndirectCommand);
		recorder.vk_cmd_buffer().drawIndexedIndirect(_indirect_buffer.vk_buffer(), offset * cmd_size, size, cmd_size);
	}
}

void SceneRenderer::compute_dependencies(DependencyGraphNode& self) {
	self.add_dependency(_cull.as_ptr());
}

}
