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

#include "CmdBufferRecorder.h"

namespace yave {

CmdBufferRecorder::CmdBufferRecorder(CmdBuffer&& buffer) : _cmd_buffer(std::move(buffer)), _render_pass(nullptr) {
	Y_TODO(optimise disposable buffers)
	vk_cmd_buffer().begin(vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
		);
}

CmdBufferRecorder::CmdBufferRecorder(CmdBufferRecorder&& other) {
	swap(other);
}

CmdBufferRecorder& CmdBufferRecorder::operator=(CmdBufferRecorder&& other) {
	swap(other);
	return *this;
}

void CmdBufferRecorder::swap(CmdBufferRecorder& other) {
	std::swap(_cmd_buffer, other._cmd_buffer);
	std::swap(_render_pass, other._render_pass);

}

vk::CommandBuffer CmdBufferRecorder::vk_cmd_buffer() const {
	return _cmd_buffer.vk_cmd_buffer();
}


const RenderPass& CmdBufferRecorder::current_pass() const {
	return *_render_pass;
}

const Viewport& CmdBufferRecorder::viewport() const {
	return _viewport;
}

RecordedCmdBuffer CmdBufferRecorder::end() {
	end_render_pass();
	vk_cmd_buffer().end();
	return RecordedCmdBuffer(std::move(_cmd_buffer));
}


void CmdBufferRecorder::end_render_pass() {
	if(_render_pass) {
		vk_cmd_buffer().endRenderPass();
		_render_pass = nullptr;
	}
}

CmdBufferRecorder::~CmdBufferRecorder() {
	if(_cmd_buffer.vk_cmd_buffer()) {
		fatal("CmdBufferRecorder destroyed before end() was called");
	}
}




CmdBufferRecorder& CmdBufferRecorder::set_viewport(const Viewport& view) {
	_viewport = view;

	return *this;
}

CmdBufferRecorder& CmdBufferRecorder::bind_framebuffer(const Framebuffer& framebuffer) {
	//vk::ClearValue clear_values[] = {vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}), vk::ClearDepthStencilValue(1.0f, 0)};
	auto clear_values =
			core::range(usize(0), framebuffer.attachment_count()).map([](usize) { return vk::ClearValue(vk::ClearColorValue(std::array<float, 4>{{0.0f, 0.0f, 0.0f, 0.0f}})); }).collect<core::Vector>() +
			vk::ClearDepthStencilValue(1.0f, 0);

	auto pass_info = vk::RenderPassBeginInfo()
			.setRenderArea(vk::Rect2D({0, 0}, {framebuffer.size().x(), framebuffer.size().y()}))
			.setRenderPass(framebuffer.render_pass().vk_render_pass())
			.setFramebuffer(framebuffer.vk_framebuffer())
			.setPClearValues(clear_values.begin())
			.setClearValueCount(u32(clear_values.size()))
		;
	vk_cmd_buffer().beginRenderPass(pass_info, vk::SubpassContents::eInline);
	_render_pass = &framebuffer.render_pass();

	return *this;
}

CmdBufferRecorder& CmdBufferRecorder::bind_pipeline(const GraphicPipeline& pipeline, const DescriptorSet& m, const DescriptorSet& vp) {
	vk_cmd_buffer().bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.vk_pipeline());

	/*std::initializer_list<vk::DescriptorSet> sets = {m.vk_descriptor_set(), vp.vk_descriptor_set(), pipeline.vk_descriptor_set()};
	vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.vk_pipeline_layout(), 0, sets.size(), sets.begin(), 0, nullptr);*/
	Y_TODO(descriptor set binding infecient)
	if(m.vk_descriptor_set()) {
		auto vk = m.vk_descriptor_set();
		vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.vk_pipeline_layout(), 0, 1, &vk, 0, nullptr);
	}
	if(vp.vk_descriptor_set()) {
		auto vk = vp.vk_descriptor_set();
		vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.vk_pipeline_layout(), 1, 1, &vk, 0, nullptr);
	}
	if(pipeline.vk_descriptor_set()) {
		auto vk = pipeline.vk_descriptor_set();
		vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.vk_pipeline_layout(), 2, 1, &vk, 0, nullptr);
	}

	return *this;
}

CmdBufferRecorder& CmdBufferRecorder::draw(const StaticMeshInstance& mesh_instance) {
	vk::DeviceSize offset = 0; // fohkin' vk::ArrayProxy
	vk_cmd_buffer().bindVertexBuffers(0, mesh_instance.vertex_buffer.vk_buffer(), offset);
	vk_cmd_buffer().bindIndexBuffer(mesh_instance.triangle_buffer.vk_buffer(), offset, vk::IndexType::eUint32);

	//vk_cmd_buffer().drawIndexed(mesh_instance.triangle_buffer.size() * 3, 1, 0, 0, 0);

	auto cmds = u32(mesh_instance.indirect_buffer.size());
	vk_cmd_buffer().drawIndexedIndirect(mesh_instance.indirect_buffer.vk_buffer(), offset, cmds, cmds ? sizeof(vk::DrawIndexedIndirectCommand) : 0);

	return *this;
}


CmdBufferRecorder& CmdBufferRecorder::dispatch(const ComputeProgram& program, const math::Vec3ui& size, const DescriptorSet& descriptor_set) {
	end_render_pass();

	auto ds = descriptor_set.vk_descriptor_set();

	vk_cmd_buffer().bindPipeline(vk::PipelineBindPoint::eCompute, program.vk_pipeline());
	vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eCompute, program.vk_pipeline_layout(), 0, 1, &ds, 0, nullptr);
	vk_cmd_buffer().dispatch(size.x(), size.y(), size.z());
	return *this;
}

}
