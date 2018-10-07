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


namespace yave {

static constexpr usize batch_size = 128 * 1024;

SceneRenderer::SceneRenderer(DevicePtr dptr, SceneView& view) :
		SecondaryRenderer(dptr),
		_view(view),
		_camera_buffer(device(), 1),
		_attrib_buffer(batch_size),
		_camera_set(device(), {Binding(_camera_buffer)}) {

	log_msg(fmt("Allocating % KB of scene buffers", (batch_size / 1024) * (sizeof(math::Transform<>) + sizeof(vk::DrawIndexedIndirectCommand))));
}

const SceneView& SceneRenderer::scene_view() const {
	return _view;
}

void SceneRenderer::build_frame_graph(FrameGraphNode&) {
}

void SceneRenderer::pre_render(CmdBufferRecorder<>& recorder, const FrameToken& token) {
	auto region = recorder.region("SceneRenderer::pre_render");

	{
		auto camera_mapping = TypedMapping(_camera_buffer);
		camera_mapping[0] = scene_view().camera().viewproj_matrix();
	}

	auto _attrib_subbuffer = _attrib_buffer[token];
	if(_attrib_subbuffer.size() < scene_view().scene().renderables().size() + scene_view().scene().static_meshes().size()) {
		y_fatal("SceneRenderer::_attrib_buffer overflow");
	}

	auto attrib_mapping = TypedMapping(_attrib_subbuffer);
	u32 attrib_index = 0;

	// renderables
	for(const auto& r : scene_view().scene().renderables()) {
		attrib_mapping[attrib_index++] = r->transform();
	}

	// static meshes
	for(const auto& r : scene_view().scene().static_meshes()) {
		attrib_mapping[attrib_index++] = r->transform();
	}
}

void SceneRenderer::render(RenderPassRecorder& recorder, const FrameToken& token) {
	auto region = recorder.region("SceneRenderer::render");

	u32 attrib_index = 0;

#warning clean unnecessary buffer binding
	auto _attrib_subbuffer = _attrib_buffer[token];
	recorder.bind_attrib_buffers({_attrib_subbuffer, _attrib_subbuffer});

	// renderables
	{
		for(const auto& r : scene_view().scene().renderables()) {
			r->render(token, recorder, Renderable::SceneData{_camera_set, attrib_index++});
		}
	}

	// static meshes
	{
		for(const auto& r : scene_view().scene().static_meshes()) {
			r->render(token, recorder, Renderable::SceneData{_camera_set, attrib_index++});
		}
	}
}

}
