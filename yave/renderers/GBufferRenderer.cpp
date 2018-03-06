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

#include "GBufferRenderer.h"

namespace yave {
namespace experimental {

GBufferRenderer::GBufferRenderer(const Ptr<SceneRenderer>& scene, const math::Vec2ui &size) :
		Renderer(scene->device()),
		_scene(scene),
		_depth(device(), depth_format, size),
		_color(device(), diffuse_format, size),
		_normal(device(), normal_format, size),
		_gbuffer(device(), _depth, {_color, _normal}) {
}

TextureView GBufferRenderer::depth() const {
	return _depth;
}

TextureView GBufferRenderer::albedo_metallic() const {
	return _color;
}

TextureView GBufferRenderer::normal_roughness() const {
	return _normal;
}

void GBufferRenderer::build_frame_graph(FrameGraphNode& frame_graph) {
	frame_graph.schedule(_scene);
}

void GBufferRenderer::render(CmdBufferRecorder<>& recorder, const FrameToken& token) {
	auto region = recorder.region("GBufferRenderer::render");

	auto pass = recorder.bind_framebuffer(_gbuffer);
	_scene->render(pass, token);
}

}
}
