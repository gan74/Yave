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

SceneRenderer::SceneRenderer(DevicePtr dptr, const Ptr<CullingNode>& cull) :
		SecondaryRenderer(dptr),
		_cull(cull),

		_camera_buffer(device(), 1),
		_camera_set(device(), {Binding(_camera_buffer)}),
		_attrib_buffer(device(), batch_size) {

	log_msg("Allocating "_s + (batch_size / 1024) * (sizeof(math::Transform<>) + sizeof(vk::DrawIndexedIndirectCommand)) + "KB of scene buffers");
}

void SceneRenderer::build_frame_graph(RenderingNode<result_type>& node, const Framebuffer& framebuffer) {
	auto culling = node.add_dependency(_cull);

	node.set_func([=, token = node.token(), &framebuffer]() {
			CmdBufferRecorder<CmdBufferUsage::Secondary> recorder(device()->create_secondary_cmd_buffer(), framebuffer);

			_camera_buffer.map()[0] = scene_view().camera().viewproj_matrix();
			const auto& results = culling.get();


			auto attrib_mapping = _attrib_buffer.map();
			AttribData attrib_data{0, attrib_mapping.size(), attrib_mapping.begin()};

			render_renderables(recorder, token, results.renderables, attrib_data);

			if(!results.static_meshes.is_empty()) {
				fatal("Static meshes not supported");
			}

			return std::move(recorder);
		});
}




usize SceneRenderer::max_size(usize size, AttribData& attrib_data) {
	if(size > attrib_data.size) {
		log_msg("Batch size exceeds scene buffer size", Log::Warning);
		return attrib_data.size;
	}
	return size;
}

void SceneRenderer::render_renderables(Recorder& recorder,
									   const FrameToken& token,
									   const core::Vector<const Renderable*>& renderables,
									   AttribData& attrib_data) {

	usize size = max_size(renderables.size(), attrib_data);

	for(usize i = 0; i != size; ++i) {
		attrib_data.begin[i] = renderables[i]->transform();
	}

	for(usize i = 0; i != size; ++i) {
		usize offset = attrib_data.offset + i;
		AttribSubBuffer attribs(_attrib_buffer, offset * sizeof(math::Transform<>), sizeof(math::Transform<>));
		renderables[i]->render(token, recorder, Renderable::SceneData{_camera_set, attribs});
	}

	attrib_data.offset += size;
	attrib_data.size -= size;
	attrib_data.begin += size;
}

}
