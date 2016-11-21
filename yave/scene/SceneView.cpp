/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

	float ratio = 4.0 / 3.0;
	mvp.proj = math::perspective(math::to_rad(45), ratio, 0.001f, 10.f);
	mvp.view = math::look_at(math::Vec3(2.0, 0, 0), math::Vec3());
}

RecordedCmdBuffer SceneView::command_buffer(const Framebuffer& fbo) {
	auto recorder = CmdBufferRecorder(_command_pool.create_buffer());
	recorder.bind_framebuffer(fbo);
	recorder.set_viewport(Viewport(fbo.size()));

	for(const auto& mesh : _scene.static_meshes()) {
		mesh.draw(recorder, _matrix_set);
	}

	return recorder.end();
}

}
