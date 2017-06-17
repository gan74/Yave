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
		SecondaryRenderer(dptr),
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

RecordedCmdBuffer<CmdBufferUsage::Secondary> SceneRenderer::process(const FrameToken& token, const Framebuffer& framebuffer) {
	CmdBufferRecorder<CmdBufferUsage::Secondary> recorder(device()->create_secondary_cmd_buffer(), framebuffer);

	_camera_mapping[0] = scene_view().camera().viewproj_matrix();
	auto results = _cull->process(token);

	render_static_meshes(recorder, results.static_meshes);
	render_renderables(recorder, token, results.renderables);

	return std::move(recorder);
}

void SceneRenderer::render_renderables(Recorder& recorder, const FrameToken& token, const core::Vector<const Renderable*>& renderables) {
	if(renderables.is_empty()) {
		return;
	}

	for(const Renderable* r : renderables) {
		r->render(token, recorder, {_camera_set});
	}
}

void SceneRenderer::render_static_meshes(Recorder& recorder, const core::Vector<const StaticMesh*>& meshes) {
	if(meshes.is_empty()) {
		return;
	}

	if(meshes.size() > batch_size) {
		log_msg("Visible object count exceeds scene buffer size", Log::Warning);
	}

	usize i = 0;
	usize submitted = 0;
	AssetPtr<Material> current_material;
	std::array<vk::Buffer, 2> current_buffers;

	for(usize count = std::min(batch_size, meshes.size()); i != count; ++i) {
		const auto& mesh = meshes[i];

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
	submit_batches(recorder, meshes[submitted]->material(), submitted, i - submitted);
}

void SceneRenderer::setup_instance(Recorder& recorder, const AssetPtr<StaticMeshInstance>& instance) {
	recorder.bind_buffers(instance->triangle_buffer,
						  {SubBuffer<BufferUsage::AttributeBit>(instance->vertex_buffer),
						   SubBuffer<BufferUsage::AttributeBit>(_matrix_buffer)});
}

void SceneRenderer::submit_batches(Recorder& recorder, AssetPtr<Material>& mat, usize offset, usize size) {
	if(size) {
		recorder.bind_material(*mat, {_camera_set});

		const usize cmd_size = sizeof(vk::DrawIndexedIndirectCommand);
		recorder.vk_cmd_buffer().drawIndexedIndirect(_indirect_buffer.vk_buffer(), offset * cmd_size, size, cmd_size);
	}
}

}
