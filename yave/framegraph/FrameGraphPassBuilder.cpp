/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include "FrameGraphPassBuilder.h"
#include "FrameGraph.h"

namespace yave {

FrameGraphPassBuilder::FrameGraphPassBuilder(FrameGraphPass* pass) : _pass(pass) {
}

void FrameGraphPassBuilder::set_render_func(FrameGraphPass::render_func&& func) {
	_pass->_render = std::move(func);
}



// --------------------------------- Framebuffer ---------------------------------

void FrameGraphPassBuilder::add_texture_input(FrameGraphImageId res, PipelineStage stage) {
	add_to_pass(res, ImageUsage::TextureBit, stage);
}

void FrameGraphPassBuilder::add_depth_output(FrameGraphMutableImageId res) {
	// transition is done by the renderpass
	add_to_pass(res, ImageUsage::DepthBit, PipelineStage::None);
	if(_pass->_depth.is_valid()) {
		y_fatal("Pass already has a depth output.");
	}
	_pass->_depth = res;
}

void FrameGraphPassBuilder::add_color_output(FrameGraphMutableImageId res) {
	// transition is done by the renderpass
	add_to_pass(res, ImageUsage::ColorBit, PipelineStage::None);
	_pass->_colors << res;
}


// --------------------------------- Copies ---------------------------------

void FrameGraphPassBuilder::add_copy_src(FrameGraphImageId res) {
	add_to_pass(res, ImageUsage::TransferSrcBit, PipelineStage::TransferBit);
}


// --------------------------------- Storage output ---------------------------------

void FrameGraphPassBuilder::add_storage_output(FrameGraphMutableImageId res, usize ds_index, PipelineStage stage) {
	add_to_pass(res, ImageUsage::StorageBit, stage);
	add_uniform(FrameGraphDescriptorBinding::create_storage_binding(res), ds_index);
}

void FrameGraphPassBuilder::add_storage_output(FrameGraphMutableBufferId res, usize ds_index, PipelineStage stage) {
	add_to_pass(res, BufferUsage::StorageBit, stage);
	add_uniform(FrameGraphDescriptorBinding::create_storage_binding(res), ds_index);
}


// --------------------------------- Storage intput ---------------------------------

void FrameGraphPassBuilder::add_storage_input(FrameGraphBufferId res, usize ds_index, PipelineStage stage) {
	add_to_pass(res, BufferUsage::StorageBit, stage);
	add_uniform(FrameGraphDescriptorBinding::create_storage_binding(res), ds_index);
}

void FrameGraphPassBuilder::add_storage_input(FrameGraphImageId res, usize ds_index, PipelineStage stage) {
	add_to_pass(res, ImageUsage::StorageBit, stage);
	add_uniform(FrameGraphDescriptorBinding::create_storage_binding(res), ds_index);
}


// --------------------------------- Uniform input ---------------------------------

void FrameGraphPassBuilder::add_uniform_input(FrameGraphBufferId res, usize ds_index, PipelineStage stage) {
	add_to_pass(res, BufferUsage::UniformBit, stage);
	add_uniform(FrameGraphDescriptorBinding::create_uniform_binding(res), ds_index);
}

void FrameGraphPassBuilder::add_uniform_input(FrameGraphImageId res, usize ds_index, PipelineStage stage) {
	add_to_pass(res, ImageUsage::TextureBit, stage);
	add_uniform(FrameGraphDescriptorBinding::create_uniform_binding(res), ds_index);
}


// --------------------------------- External ---------------------------------

Y_TODO(external framegraph resources are not synchronized)
void FrameGraphPassBuilder::add_uniform_input(StorageView tex, usize ds_index, PipelineStage) {
	add_uniform(Binding(tex), ds_index);
}

void FrameGraphPassBuilder::add_uniform_input(TextureView tex, usize ds_index, PipelineStage) {
	add_uniform(Binding(tex), ds_index);
}

void FrameGraphPassBuilder::add_uniform_input(CubemapView tex, usize ds_index, PipelineStage) {
	add_uniform(Binding(tex), ds_index);
}


// --------------------------------- Attribs ---------------------------------

void FrameGraphPassBuilder::add_attrib_input(FrameGraphBufferId res, PipelineStage stage) {
	add_to_pass(res, BufferUsage::AttributeBit, stage);
}

void FrameGraphPassBuilder::add_index_input(FrameGraphBufferId res, PipelineStage stage) {
	add_to_pass(res, BufferUsage::IndexBit, stage);
}


// --------------------------------- stuff ---------------------------------

void FrameGraphPassBuilder::add_descriptor_binding(Binding bind, usize ds_index) {
	add_uniform(bind, ds_index);
}

void FrameGraphPassBuilder::add_to_pass(FrameGraphImageId res, ImageUsage usage, PipelineStage stage) {
	res.check_valid();
	auto& info = _pass->_images[res];
	info.stage = stage & stage;
	_pass->_parent->add_usage(res, usage);
}

void FrameGraphPassBuilder::add_to_pass(FrameGraphBufferId res, BufferUsage usage, PipelineStage stage) {
	res.check_valid();
	auto& info = _pass->_buffers[res];
	info.stage = stage & stage;
	_pass->_parent->add_usage(res, usage);
}

void FrameGraphPassBuilder::add_uniform(FrameGraphDescriptorBinding binding, usize ds_index) {
	auto& bindings = _pass->_bindings;
	while(bindings.size() <= ds_index) {
		bindings.emplace_back();
	}
	bindings[ds_index].push_back(binding);
}

void FrameGraphPassBuilder::set_cpu_visible(FrameGraphMutableBufferId res) {
	_pass->_parent->set_cpu_visible(res);
}

}
