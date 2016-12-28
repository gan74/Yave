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
#ifndef YAVE_COMMANDS_CMDBUFFERRECORDER_H
#define YAVE_COMMANDS_CMDBUFFERRECORDER_H

#include <yave/yave.h>
#include <yave/Framebuffer.h>

#include "RecordedCmdBuffer.h"

#include <yave/mesh/StaticMeshInstance.h>
#include <yave/material/GraphicPipeline.h>
#include <yave/shaders/ComputeProgram.h>
#include <yave/descriptors/DescriptorSet.h>
#include <yave/Viewport.h>

namespace yave {

class CmdBufferRecorder : NonCopyable {

	public:
		CmdBufferRecorder(CmdBuffer&& buffer);

		CmdBufferRecorder(CmdBufferRecorder&& other);
		CmdBufferRecorder& operator=(CmdBufferRecorder&& other);

		RecordedCmdBuffer end();
		~CmdBufferRecorder();

		vk::CommandBuffer vk_cmd_buffer() const;

		const RenderPass& current_pass() const;
		const Viewport& viewport() const;

		CmdBufferRecorder& set_viewport(const Viewport& view);
		CmdBufferRecorder& bind_framebuffer(const Framebuffer& framebuffer);
		CmdBufferRecorder& bind_pipeline(const GraphicPipeline& pipeline, const DescriptorSet& m, const DescriptorSet& vp);
		CmdBufferRecorder& draw(const StaticMeshInstance& mesh_instance);
		CmdBufferRecorder& dispatch(const ComputeProgram& program, const math::Vec3ui& size, const DescriptorSet& descriptor_set);

	private:
		void swap(CmdBufferRecorder& other);

		void end_render_pass();

		CmdBuffer _cmd_buffer;
		const RenderPass* _render_pass;
		Viewport _viewport;
};

}

#endif // YAVE_COMMANDS_CMDBUFFERRECORDER_H
