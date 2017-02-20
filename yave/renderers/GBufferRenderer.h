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

#include <yave/pipeline/Node.h>
#include <yave/scene/SceneView.h>

namespace yave {

class GBufferRenderer : public Node, public DeviceLinked {

	public:
		static constexpr vk::Format depth_format = vk::Format::eD32Sfloat;
		static constexpr vk::Format diffuse_format = vk::Format::eR8G8B8A8Unorm;
		static constexpr vk::Format normal_format = vk::Format::eR8G8B8A8Unorm;

		template<typename N>
		GBufferRenderer(DevicePtr dptr, const math::Vec2ui& size, N& node) :
				GBufferRenderer(dptr, size, node, node.scene_view()) {
		}

		const math::Vec2ui& size() const;
		const SceneView& scene_view() const;


		const DepthTextureAttachment& depth() const;
		const ColorTextureAttachment& diffuse() const;
		const ColorTextureAttachment& normal() const;


		virtual core::ArrayProxy<Node*> dependencies() override;
		virtual void process(const FrameToken& token, CmdBufferRecorder<>&recorder) override;

	private:	
		GBufferRenderer(DevicePtr dptr, const math::Vec2ui& size, Node& child, const SceneView& view);

		Node& _child;
		const SceneView& _scene_view;

		math::Vec2ui _size;

		DepthTextureAttachment _depth;
		ColorTextureAttachment _diffuse;
		ColorTextureAttachment _normal;

		Framebuffer _gbuffer;
};


}

#endif // YAVE_RENDERERS_GBUFFERRENDERER_H
