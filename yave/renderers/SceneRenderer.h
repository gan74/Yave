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
#ifndef YAVE_RENDERERS_SCENERENDERER_H
#define YAVE_RENDERERS_SCENERENDERER_H

#include "CullingNode.h"
#include <yave/bindings/DescriptorSet.h>

namespace yave {

class SceneRenderer : public SecondaryRenderer {

	using Recorder = CmdBufferRecorder<CmdBufferUsage::Secondary>;

	public:
		SceneRenderer(DevicePtr dptr, const Ptr<CullingNode>& cull);
		void build_frame_graph(RenderingNode<result_type>& node, const Framebuffer& framebuffer) override;


		const SceneView& scene_view() const {
			return _cull->scene_view();
		}

		const auto& culling_node() const {
			return _cull;
		}

	private:
		struct AttribData {
			usize offset;
			usize size;
			math::Matrix4<>* data;
		};

		void render_renderables(Recorder& recorder, const FrameToken& token, const core::Vector<const Renderable*>& renderables, AttribData& attrib_data);

		Ptr<CullingNode> _cull;

		TypedBuffer<uniform::ViewProj, BufferUsage::UniformBit> _camera_buffer;
		DescriptorSet _camera_set;

		TypedBuffer<math::Matrix4<>, BufferUsage::AttributeBit, MemoryType::CpuVisible> _attrib_buffer;
};

}

#endif // YAVE_RENDERERS_SCENERENDERER_H
