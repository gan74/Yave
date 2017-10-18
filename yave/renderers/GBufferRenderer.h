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
#ifndef YAVE_RENDERERS_GBUFFERRENDERER_H
#define YAVE_RENDERERS_GBUFFERRENDERER_H

#include "SceneRenderer.h"

namespace yave {

class GBufferRenderer : public BufferRenderer {

	public:
		static constexpr vk::Format depth_format = vk::Format::eD32Sfloat;
		static constexpr vk::Format diffuse_format = vk::Format::eR8G8B8A8Unorm;
		static constexpr vk::Format normal_format = vk::Format::eR16G16B16A16Unorm;


		GBufferRenderer(DevicePtr dptr, const math::Vec2ui& size, const Ptr<CullingNode>& node);
		void build_frame_graph(RenderingNode<result_type>& node, CmdBufferRecorder<>& recorder) override;


		const DepthTextureAttachment& depth() const;
		const ColorTextureAttachment& color() const;
		const ColorTextureAttachment& normal() const;

		const math::Vec2ui& size() const;


		const SceneView& scene_view() const {
			return _scene->scene_view();
		}

		const auto& scene_renderer() const {
			return _scene;
		}

	private:
		core::Arc<SceneRenderer> _scene;

		DepthTextureAttachment _depth;
		ColorTextureAttachment _color;
		ColorTextureAttachment _normal;

		Framebuffer _gbuffer;
};


}

#endif // YAVE_RENDERERS_GBUFFERRENDERER_H
