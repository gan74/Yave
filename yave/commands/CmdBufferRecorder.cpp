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
	get_vk_cmd_buffer().begin(vk::CommandBufferBeginInfo()
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

vk::CommandBuffer CmdBufferRecorder::get_vk_cmd_buffer() const {
	return _cmd_buffer.get_vk_cmd_buffer();
}


const RenderPass& CmdBufferRecorder::get_current_pass() const {
	return *_render_pass;
}

const Viewport& CmdBufferRecorder::get_viewport() const {
	return _viewport;
}

RecordedCmdBuffer CmdBufferRecorder::end() {
	if(_render_pass) {
		get_vk_cmd_buffer().endRenderPass();
	}
	get_vk_cmd_buffer().end();
	return RecordedCmdBuffer(std::move(_cmd_buffer));
}

CmdBufferRecorder::~CmdBufferRecorder() {
	if(_cmd_buffer.get_vk_cmd_buffer()) {
		fatal("CmdBufferRecorder destroyed before end() was called");
	}
}




CmdBufferRecorder& CmdBufferRecorder::set_viewport(const Viewport& view) {
	_viewport = view;

	return *this;
}

CmdBufferRecorder& CmdBufferRecorder::bind_framebuffer(const Framebuffer& framebuffer) {
	vk::ClearValue clear_values[] = {vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}), vk::ClearDepthStencilValue(1.0f, 0)};
	auto pass_info = vk::RenderPassBeginInfo()
			.setRenderArea(vk::Rect2D({0, 0}, {framebuffer.size().x(), framebuffer.size().y()}))
			.setRenderPass(framebuffer.get_render_pass().get_vk_render_pass())
			.setFramebuffer(framebuffer.get_vk_framebuffer())
			.setPClearValues(clear_values)
			.setClearValueCount(2)
		;
	get_vk_cmd_buffer().beginRenderPass(pass_info, vk::SubpassContents::eInline);
	_render_pass = &framebuffer.get_render_pass();

	return *this;
}

CmdBufferRecorder& CmdBufferRecorder::bind_pipeline(const GraphicPipeline& pipeline, const DescriptorSet& m, const DescriptorSet& vp) {
	get_vk_cmd_buffer().bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get_vk_pipeline());

	/*std::initializer_list<vk::DescriptorSet> sets = {m.get_vk_descriptor_set(), vp.get_vk_descriptor_set(), pipeline.get_vk_descriptor_set()};
	get_vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.get_vk_pipeline_layout(), 0, sets.size(), sets.begin(), 0, nullptr);*/
	Y_TODO(descriptor set binding infecient)
	if(m.get_vk_descriptor_set()) {
		auto vk = m.get_vk_descriptor_set();
		get_vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.get_vk_pipeline_layout(), 0, 1, &vk, 0, nullptr);
	}
	if(vp.get_vk_descriptor_set()) {
		auto vk = vp.get_vk_descriptor_set();
		get_vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.get_vk_pipeline_layout(), 1, 1, &vk, 0, nullptr);
	}
	if(pipeline.get_vk_descriptor_set()) {
		auto vk = pipeline.get_vk_descriptor_set();
		get_vk_cmd_buffer().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.get_vk_pipeline_layout(), 2, 1, &vk, 0, nullptr);
	}

	return *this;
}

CmdBufferRecorder& CmdBufferRecorder::draw(const StaticMeshInstance& static_mesh) {
	vk::DeviceSize offset = 0; // fohkin vk::ArrayProxy
	get_vk_cmd_buffer().bindVertexBuffers(0, static_mesh.vertex_buffer.get_vk_buffer(), offset);
	get_vk_cmd_buffer().bindIndexBuffer(static_mesh.triangle_buffer.get_vk_buffer(), offset, vk::IndexType::eUint32);
	get_vk_cmd_buffer().drawIndexed(static_mesh.triangle_buffer.size() * 3, 1, 0, 0, 0);

	return *this;
}

}
