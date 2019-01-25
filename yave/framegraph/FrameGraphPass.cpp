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

#include "FrameGraphPass.h"
#include "FrameGraph.h"

namespace yave {

FrameGraphPass::FrameGraphPass(std::string_view name, FrameGraph* parent) : _name(name), _parent(parent) {
}

const core::String& FrameGraphPass::name() const {
	return _name;
}

const FrameGraphResourcePool* FrameGraphPass::resources() const {
	return _parent->resources();
}

const Framebuffer& FrameGraphPass::framebuffer() const {
	if(!_framebuffer.device()) {
		y_fatal("Pass has no framebuffer.");
	}
	return _framebuffer;
}

core::ArrayView<DescriptorSet> FrameGraphPass::descriptor_sets() const {
	return _descriptor_sets;
}

void FrameGraphPass::render(CmdBufferRecorder& recorder) const {
	_render(recorder, this);
}

void FrameGraphPass::init_framebuffer(FrameGraphResourcePool* pool) {
	y_profile();
	if(_depth.is_valid() || _colors.size()) {
		DepthAttachmentView depth;
		if(_depth.is_valid()) {
			depth = pool->image<ImageUsage::DepthBit>(_depth);
		}
		auto colors = core::vector_with_capacity<ColorAttachmentView>(_colors.size());
		for(auto&& color : _colors) {
			colors << pool->image<ImageUsage::ColorBit>(color);
		}
		_framebuffer = Framebuffer(pool->device(), depth, colors);
	}
}

void FrameGraphPass::init_descriptor_sets(FrameGraphResourcePool* pool) {
	y_profile();
	for(const auto& set : _bindings) {
		auto bindings = core::vector_with_capacity<Binding>(set.size());
		std::transform(set.begin(), set.end(), std::back_inserter(bindings), [=](const FrameGraphDescriptorBinding& b) { return b.create_binding(pool); });
		_descriptor_sets << DescriptorSet(pool->device(), bindings);
	}
}

}
