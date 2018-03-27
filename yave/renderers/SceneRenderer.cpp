/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include <yave/buffers/TypedMapping.h>

namespace yave {

static constexpr usize batch_size = 128 * 1024;

SceneRenderer::SceneRenderer(DevicePtr dptr, SceneView& view) :
		SecondaryRenderer(dptr),
		_view(view),
		_camera_buffer(device(), 1),
		_attrib_buffer(device(), batch_size),
		_camera_set(device(), {Binding(_camera_buffer)}) {

	log_msg("Allocating "_s + (batch_size / 1024) * (sizeof(math::Transform<>) + sizeof(vk::DrawIndexedIndirectCommand)) + "KB of scene buffers");
}

const SceneView& SceneRenderer::scene_view() const {
	return _view;
}

void SceneRenderer::build_frame_graph(FrameGraphNode&) {
}

void SceneRenderer::pre_render(CmdBufferRecorder<>& recorder, const FrameToken&) {
	auto region = recorder.region("SceneRenderer::pre_render");

	{
		auto camera_mapping = TypedMapping(_camera_buffer);
		camera_mapping[0] = scene_view().camera().viewproj_matrix();
	}

	const auto& renderables = scene_view().scene().renderables();
	{
		auto attrib_mapping = TypedMapping(_attrib_buffer);
		for(usize i = 0; i != renderables.size(); ++i) {
			attrib_mapping[i] = renderables[i]->transform();
		}
	}
}

void SceneRenderer::render(RenderPassRecorder& recorder, const FrameToken& token) {
	auto region = recorder.region("SceneRenderer::render");

	const auto& renderables = scene_view().scene().renderables();
	for(usize i = 0; i != renderables.size(); ++i) {
		AttribSubBuffer<math::Transform<>> attribs(_attrib_buffer, i, 1);
		renderables[i]->render(token, recorder, Renderable::SceneData{_camera_set, attribs});
	}
}

}
