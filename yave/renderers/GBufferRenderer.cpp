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

#include "GBufferRenderer.h"

namespace yave {

GBufferRenderer::GBufferRenderer(DevicePtr dptr, const math::Vec2ui &size, const Ptr<CullingNode>& node) :
		BufferRenderer(dptr),
		_scene(dptr, node),
		_depth(device(), depth_format, size),
		_color(device(), diffuse_format, size),
		_normal(device(), normal_format, size),
		_gbuffer(device(), _depth, {_color, _normal}) {
}

const SceneView& GBufferRenderer::scene_view() const {
	return _scene.scene_view();
}

const DepthTextureAttachment& GBufferRenderer::depth() const {
	return _depth;
}

const ColorTextureAttachment& GBufferRenderer::color() const {
	return _color;
}

const ColorTextureAttachment& GBufferRenderer::normal() const {
	return _normal;
}

const math::Vec2ui& GBufferRenderer::size() const {
	return _color.size();
}

TextureView GBufferRenderer::process(const FrameToken& token, CmdBufferRecorder<>& recorder) {
	RecordedCmdBuffer<CmdBufferUsage::Secondary> cmd_buffer = _scene.process(token, _gbuffer);

	recorder.execute(std::move(cmd_buffer), _gbuffer);

	return _color;
}

}
