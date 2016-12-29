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
#ifndef YAVE_SCENE_SCENEVIEW_H
#define YAVE_SCENE_SCENEVIEW_H

#include <yave/commands/RecordedCmdBuffer.h>
#include <yave/commands/CmdBufferPool.h>

#include <yave/Framebuffer.h>

#include <yave/buffer/TypedBuffer.h>

#include "Scene.h"

namespace yave {

class SceneView : NonCopyable {

	public:
		struct Matrices {
			math::Matrix4<> view;
			math::Matrix4<> proj;
		};

		SceneView(DevicePtr dptr, const Scene& sce);

		RecordedCmdBuffer command_buffer(const Framebuffer& fbo);

		void set_view(const math::Matrix4<>& view);
		void set_proj(const math::Matrix4<>& proj);

		const Scene& scene() const;

	private:
		auto map() {
			return _matrix_buffer.map();
		}

		const Scene& _scene;

		CmdBufferPool _command_pool;

		TypedBuffer<Matrices, BufferUsage::UniformBuffer> _matrix_buffer;
		DescriptorSet _matrix_set;
};

}

#endif // YAVE_SCENE_SCENEVIEW_H
