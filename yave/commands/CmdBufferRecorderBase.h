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
#ifndef YAVE_COMMANDS_CMDBUFFERRECORDERBASE_H
#define YAVE_COMMANDS_CMDBUFFERRECORDERBASE_H

#include <yave/yave.h>
#include <yave/framebuffers/Framebuffer.h>
#include <yave/barriers/Barrier.h>

#include "CmdBuffer.h"

#include <yave/mesh/StaticMeshInstance.h>
#include <yave/material/GraphicPipeline.h>
#include <yave/shaders/ComputeProgram.h>
#include <yave/bindings/DescriptorSet.h>
#include <yave/framebuffers/Viewport.h>


namespace yave {

class CmdBufferRecorderBase : NonCopyable {

	public:
		vk::CommandBuffer vk_cmd_buffer() const;

		const RenderPass& current_pass() const;

		void end_render_pass();

		void set_viewport(const Viewport& view);
		void bind_framebuffer(const Framebuffer& framebuffer);
		void bind_pipeline(const GraphicPipeline& pipeline, std::initializer_list<std::reference_wrapper<const DescriptorSet>> descriptor_sets);
		void dispatch(const ComputeProgram& program, const math::Vec3ui& size, std::initializer_list<std::reference_wrapper<const DescriptorSet>> descriptor_sets);

		void barriers(const core::ArrayProxy<BufferBarrier>& buffers, const core::ArrayProxy<ImageBarrier>& images, PipelineStage src, PipelineStage dst);

		// never use directly, needed for internal work and image loading
		void transition_image(ImageBase& image, vk::ImageLayout src, vk::ImageLayout dst);


	protected:
		CmdBufferRecorderBase() = default;
		CmdBufferRecorderBase(CmdBufferBase&& base, CmdBufferUsage usage);

		void swap(CmdBufferRecorderBase& other);

		CmdBufferBase& cmd_buffer() {
			return _cmd_buffer;
		}

	private:
		CmdBufferBase _cmd_buffer;
		const RenderPass* _render_pass = nullptr;
};

}

#endif // YAVE_COMMANDS_CMDBUFFERRECORDERBASE_H
