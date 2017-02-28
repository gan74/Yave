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
		DeviceLinked(dptr),

		_scene(dptr, node),

		_size(size),
		_depth(device(), depth_format, _size),
		_diffuse(device(), diffuse_format, _size),
		_normal(device(), normal_format, _size),
		_gbuffer(device(), _depth, {_diffuse, _normal}) {
}

const math::Vec2ui& GBufferRenderer::size() const {
	return _size;
}

const SceneView& GBufferRenderer::scene_view() const {
	return _scene.scene_view();
}

const DepthTextureAttachment& GBufferRenderer::depth() const {
	return _depth;
}

const ColorTextureAttachment& GBufferRenderer::diffuse() const {
	return _diffuse;
}

const ColorTextureAttachment& GBufferRenderer::normal() const {
	return _normal;
}

core::Vector<Renderer::Dependency> GBufferRenderer::dependencies() {
	core::Vector<Dependency> deps;
	auto scene_deps = _scene.dependencies();
	std::transform(scene_deps.begin(), scene_deps.end(), std::back_inserter(deps), [](Node* d) { return Dependency(d, nullptr, nullptr); });
	return deps;
}

void GBufferRenderer::process(const FrameToken& token, CmdBufferRecorder<>& recorder, const SubRendererResults&) {
	recorder.bind_framebuffer(_gbuffer);

	_scene.process(token, recorder);
}


}
