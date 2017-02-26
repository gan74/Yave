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
#ifndef YAVE_RENDERERS_DEFERREDRENDERER_H
#define YAVE_RENDERERS_DEFERREDRENDERER_H

#include <yave/framebuffers/Framebuffer.h>
#include <yave/shaders/ComputeProgram.h>
#include <yave/bindings/uniforms.h>

#include <yave/scene/SceneView.h>

#include "GBufferRenderer.h"

namespace yave {

class DeferredRenderer : public Node, public DeviceLinked {

	public:
		using OutputView = decltype(FrameToken::image_view);

		DeferredRenderer(const Ptr<GBufferRenderer>& gbuffer);

		const math::Vec2ui& size() const;

		virtual core::Vector<NodePtr> dependencies() override;
		virtual void process(const FrameToken& token, CmdBufferRecorder<>& recorder) override;

	private:
		const DescriptorSet& create_output_set(const StorageView& out);

		Ptr<GBufferRenderer> _gbuffer;

		ComputeShader _lighting_shader;
		ComputeProgram _lighting_program;

		Buffer<BufferUsage::StorageBit, MemoryFlags::CpuVisible> _lights;

		TypedBuffer<uniform::Camera, BufferUsage::UniformBit> _camera_buffer;

		DescriptorSet _lighting_set;
		std::unordered_map<VkImageView, DescriptorSet> _output_sets;
};

}


#endif // YAVE_RENDERERS_DEFERREDRENDERER_H
