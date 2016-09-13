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
#ifndef YAVE_CMDBUFFERRECORDER_H
#define YAVE_CMDBUFFERRECORDER_H

#include "yave.h"
#include "Framebuffer.h"

#include <yave/buffer/BufferMemoryReference.h>
#include <yave/mesh/StaticMeshInstance.h>
#include <yave/material/GraphicPipeline.h>

namespace yave {

template<typename T>
class CmdBufferRecorder : NonCopyable {

	using Self = CmdBufferRecorder<T>&;

	public:
		CmdBufferRecorder(vk::CommandBuffer buffer) : _cmd_buffer(buffer),  _nested_passes(0) {
		}

		CmdBufferRecorder(CmdBufferRecorder&& other) : _cmd_buffer(other._cmd_buffer), _nested_passes(other._nested_passes) {
		}

		auto end() {
			for(; _nested_passes; _nested_passes--) {
				_cmd_buffer.endRenderPass();
			}
			_cmd_buffer.end();
			return T(std::exchange(_cmd_buffer, vk::CommandBuffer(VK_NULL_HANDLE)));
		}

		~CmdBufferRecorder() {
			if(_cmd_buffer) {
				fatal("CommandBufferRecorder was not ended.");
			}
		}

		vk::CommandBuffer get_vk_cmd_buffer() const {
			return _cmd_buffer;
		}




		template<MemoryFlags DstFlags, MemoryFlags SrcFlags>
		Self copy_buffer(BufferMemoryReference<DstFlags, BufferTransfer::TransferDst> dst, BufferMemoryReference<SrcFlags, BufferTransfer::TransferSrc> src) {
			_cmd_buffer.copyBuffer(src->get_vk_buffer(), dst->get_vk_buffer(), dst->get_copy());

			return *this;
		}

		Self bind_framebuffer(const Framebuffer &framebuffer) {
			vk::ClearValue clear_values[] = {vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}), vk::ClearDepthStencilValue(1.0f, 0)};
			auto pass_info = vk::RenderPassBeginInfo()
					.setRenderArea(vk::Rect2D({0, 0}, {framebuffer.size().x(), framebuffer.size().y()}))
					.setRenderPass(framebuffer.get_render_pass().get_vk_render_pass())
					.setFramebuffer(framebuffer.get_vk_framebuffer())
					.setPClearValues(clear_values)
					.setClearValueCount(2)
				;
			_cmd_buffer.beginRenderPass(pass_info, vk::SubpassContents::eInline);
			_nested_passes++;

			return *this;
		}

		Self bind_pipeline(const GraphicPipeline &pipeline) {
			_cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get_vk_pipeline());
			_cmd_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.get_vk_pipeline_layout(), 0, 1, &pipeline.get_descriptor_set().set, 0, nullptr);

			return *this;
		}

		Self draw(const StaticMeshInstance &static_mesh) {
			vk::DeviceSize offset = 0; // fohkin vk::ArrayProxy
			_cmd_buffer.bindVertexBuffers(0, static_mesh.vertex_buffer.get_vk_buffer(), offset);
			_cmd_buffer.bindIndexBuffer(static_mesh.triangle_buffer.get_vk_buffer(), offset, vk::IndexType::eUint32);
			_cmd_buffer.drawIndexed(static_mesh.triangle_buffer._size() * 3, 1, 0, 0, 0);

			return *this;
		}


	private:
		// Ownership transfered in end()
		NotOwned<vk::CommandBuffer> _cmd_buffer;
		usize _nested_passes;
};

}

#endif // YAVE_CMDBUFFERRECORDER_H
