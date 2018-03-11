/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
#ifndef YAVEL_RENDERERS_SCENERENDERER_H
#define YAVEL_RENDERERS_SCENERENDERER_H

#include "renderers.h"

#include <yave/bindings/DescriptorSet.h>
#include <yave/scene/SceneView.h>

namespace yave {
namespace experimental {

class SceneRenderer : public SecondaryRenderer {
	public:
		SceneRenderer(DevicePtr dptr, SceneView& view);

		const SceneView& scene_view() const;

		void render(RenderPassRecorder& recorder, const FrameToken& token) override;

	protected:
		void build_frame_graph(FrameGraphNode&) override;

		void pre_render(CmdBufferRecorder<>& recorder, const FrameToken&) override;

	private:
		const SceneView& _view;

		TypedBuffer<uniform::ViewProj, BufferUsage::UniformBit> _camera_buffer;

		TypedBuffer<math::Matrix4<>, BufferUsage::AttributeBit, MemoryType::CpuVisible> _attrib_buffer;

		DescriptorSet _camera_set;
};

}
}

#endif // YAVEL_RENDERERS_RENDERERS_H
