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

#include <y/core/Chrono.h>

#include <unordered_map>

namespace yave {


CullingNode::CullingNode(SceneView &view) : _view(view) {
}

const SceneView& CullingNode::scene_view() const {
	return _view;
}

CullingNode::CullingResults CullingNode::process(const FrameToken&) {
	Frustum frustum = _view.camera().frustum();

	return CullingResults{
			process_static_meshes(frustum),
			process_renderables(frustum)
		};
}

core::Vector<const Renderable*> CullingNode::process_renderables(const Frustum& frustum) {
	auto visibles = core::vector_with_capacity<const Renderable*>(_view.scene().renderables().size() / 2);

	for(const auto& r : _view.scene().renderables()) {
		if(frustum.is_inside(r->position(), r->radius())) {
			visibles.push_back(r.as_ptr());
		}
	}

	return visibles;
}

core::Vector<const StaticMesh*> CullingNode::process_static_meshes(const Frustum& frustum) {
	auto visibles = core::vector_with_capacity<const StaticMesh*>(_view.scene().static_meshes().size() / 2);

	constexpr bool use_map = true;
	if constexpr(use_map) {
		std::unordered_map<Material*, decltype(visibles)> per_mat;
		for(const auto& m : _view.scene().static_meshes()) {
			if(frustum.is_inside(m->position(), m->radius())) {
				per_mat[m->material().as_ptr()].push_back(m.as_ptr());
			}
		}
		{
			core::DebugTimer _("merge", core::Duration::milliseconds(1));
			for(const auto& mat : per_mat) {
				visibles.push_back(mat.second.begin(), mat.second.end());
			}
		}
	} else {
		for(const auto& m : _view.scene().static_meshes()) {
			if(frustum.is_inside(m->position(), m->radius())) {
				visibles.push_back(m.as_ptr());
			}
		}
		{
			core::DebugTimer _("sort", core::Duration::milliseconds(1));
			sort(visibles.begin(), visibles.end(), [](const auto& a, const auto& b) { return a->material() < b->material(); });
		}
	}

	return visibles;
}

}
