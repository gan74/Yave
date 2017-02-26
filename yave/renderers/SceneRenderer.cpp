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

SceneRenderer::SceneRenderer(DevicePtr dptr, const Node::Ptr<CullingNode>& cull):
		_cull(cull),
		_matrix_buffer(dptr, 1),
		_mapping(_matrix_buffer.map()),
		_matrix_set(dptr, {Binding(_matrix_buffer)}) {
}

const SceneView& SceneRenderer::scene_view() const {
	return _cull->scene_view();
}

core::Vector<Node::NodePtr> SceneRenderer::dependencies() {
	return {_cull};
}

void SceneRenderer::process(const FrameToken&, CmdBufferRecorder<>& recorder) {
	_mapping[0] = scene_view().camera().viewproj_matrix();

	for(const auto& mesh : _cull->visibles()) {
		mesh->draw(recorder, _matrix_set);
	}
}



}
