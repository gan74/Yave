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

template<typename T>
static auto process_transformables(const core::Vector<Scene::Ptr<T>>& objects, const Frustum& frustum) {
	auto visibles = core::vector_with_capacity<const T*>(objects.size() / 2);

	for(const auto& o : objects) {
		if(frustum.is_inside(o->position(), o->radius())) {
			visibles << o.as_ptr();
		}
	}

	return visibles;
}

static auto process_lights(const core::Vector<Scene::Ptr<Light>>& lights, const Frustum& frustum) {
	auto directionals = core::vector_with_capacity<const Light*>(4);
	auto visibles = core::vector_with_capacity<const Light*>(lights.size() / 2);

	for(const auto& l : lights) {
		if(l->type() == Light::Directional) {
			directionals << l.as_ptr();
		} else if(frustum.is_inside(l->position(), l->radius())) {
			visibles << l.as_ptr();
		}
	}

	return std::pair{directionals, visibles};
}

static auto process_static_meshes(const core::Vector<Scene::Ptr<StaticMesh>>& meshes, const Frustum& frustum) {
	auto visibles = core::vector_with_capacity<const StaticMesh*>(meshes.size() / 2);

	constexpr bool use_map = true;
	if constexpr(use_map) {
		std::unordered_map<Material*, decltype(visibles)> per_mat;
		for(const auto& m : meshes) {
			if(frustum.is_inside(m->position(), m->radius())) {
				per_mat[m->material().as_ptr()] << m.as_ptr();
			}
		}
		{
			core::DebugTimer _("merge", core::Duration::milliseconds(1));
			for(const auto& mat : per_mat) {
				visibles.push_back(mat.second.begin(), mat.second.end());
			}
		}
	} else {
		for(const auto& m : meshes) {
			if(frustum.is_inside(m->position(), m->radius())) {
				visibles << m.as_ptr();
			}
		}
		{
			core::DebugTimer _("sort", core::Duration::milliseconds(1));
			sort(visibles.begin(), visibles.end(), [](const auto& a, const auto& b) { return a->material() < b->material(); });
		}
	}

	return visibles;
}



CullingNode::CullingNode(SceneView &view) : _view(view) {
}

void CullingNode::build_frame_graph(RenderingNode<result_type>& node) {
	node.set_func([=]() {
			Frustum frustum = _view.camera().frustum();

			auto [dirs, lights] = process_lights(_view.scene().lights(), frustum);
			return CullingResults{
					process_static_meshes(_view.scene().static_meshes(), frustum),
					process_transformables(_view.scene().renderables(), frustum),
					std::move(dirs),
					std::move(lights)
				};
		});
}

}
