/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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

#include "SceneView.h"

#include <yave/commands/CmdBufferRecorder.h>

namespace yave {

SceneView::SceneView(DevicePtr dptr, const Scene &sce) :
		_scene(sce),
		_command_pool(dptr),
		_matrix_buffer(dptr, 1),
		_matrix_set(dptr, {Binding(_matrix_buffer)}) {

	auto mapping = _matrix_buffer.map();
	auto& mvp = *mapping.begin();

	float ratio = 4.0f / 3.0f;
	mvp.proj = math::perspective(math::to_rad(45), ratio, 0.001f, 10.f);
	mvp.view = math::look_at(math::Vec3(2.0, 0, 0), math::Vec3());
}

void SceneView::draw(CmdBufferRecorder& recorder) const {
	for(const auto& mesh : _scene.static_meshes()) {
		mesh.draw(recorder, _matrix_set);
	}
}

void SceneView::set_view(const math::Matrix4<>& view) {
	map().begin()->view = view;
}

void SceneView::set_proj(const math::Matrix4<>& proj) {
	map().begin()->proj = proj;
}

const Scene& SceneView::scene() const {
	return _scene;
}

DevicePtr SceneView::device() const {
	return _matrix_buffer.device();
}

}
