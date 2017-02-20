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

#include "CullingNode.h"

namespace yave {

CullingNode::CullingNode(SceneView &view) : _view(view) {
}

const SceneView& CullingNode::scene_view() const {
	return _view;
}

void CullingNode::process(const FrameToken&, CmdBufferRecorder<>&) {
	_visibles.make_empty();
	auto frustum = _view.camera().frustum();

	for(const auto& m : _view.scene().static_meshes()) {
		if(frustum.is_inside(m.position(), m.radius())) {
			_visibles.push_back(&m);
		}
	}
}


const core::Vector<const StaticMesh*>& CullingNode::visibles() const {
	return _visibles;
}

}
