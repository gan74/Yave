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

		void draw(CmdBufferRecorder& recorder) const;

		void set_view(const math::Matrix4<>& view);
		void set_proj(const math::Matrix4<>& proj);

		const Scene& scene() const;

		DevicePtr device() const;

	private:
		auto map() {
			return _matrix_buffer.map();
		}

		const Scene& _scene;

		CmdBufferPool _command_pool;

		TypedBuffer<Matrices, BufferUsage::UniformBit> _matrix_buffer;
		DescriptorSet _matrix_set;
};

}

#endif // YAVE_SCENE_SCENEVIEW_H
