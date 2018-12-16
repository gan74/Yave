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
#ifndef YAVE_FRAMEGRAPH_RENDERERS_H
#define YAVE_FRAMEGRAPH_RENDERERS_H

#include <yave/scene/SceneView.h>
#include <yave/graphics/bindings/DescriptorSet.h>

#include "FrameGraph.h"

namespace yave {

class SceneRender {
	public:
		static constexpr usize max_batch_count = 128 * 1024;

		static SceneRender create(FrameGraphPassBuilder& builder, const SceneView& view);

		void render(RenderPassRecorder& recorder, const FrameGraphResourcePool* resources) const;

	private:
		template<typename T>
		using AttribBuffer = TypedBuffer<T, BufferUsage::AttributeBit, MemoryType::CpuVisible>;

		SceneView _scene_view;

		//DescriptorSet _camera_set;

		FrameGraphResource<TypedUniformBuffer<uniform::ViewProj>> _camera_buffer;
		FrameGraphResource<AttribBuffer<math::Transform<>>> _transform_buffer;
		FrameGraphResource<DescriptorSet> _camera_set;
};


struct GBufferPass {
	SceneRender scene;

	FrameGraphResource<DepthTextureAttachment> depth;
	FrameGraphResource<ColorTextureAttachment> color;
	FrameGraphResource<ColorTextureAttachment> normal;

	FrameGraphResource<Framebuffer> gbuffer;
};

GBufferPass& render_gbuffer(FrameGraph& framegraph, const SceneView& view, const math::Vec2ui& size);


}

#endif // YAVE_FRAMEGRAPH_RENDERERS_H
