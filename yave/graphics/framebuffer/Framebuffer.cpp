/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#include "Framebuffer.h"
#include "RenderPass.h"
#include <yave/device/Device.h>

namespace yave {

static math::Vec2ui compute_size(const DepthAttachmentView& depth, core::ArrayView<ColorAttachmentView> views) {
	math::Vec2ui ref;
	if(depth.device()) {
		ref = depth.size();
	} else if(!views.is_empty()) {
		ref = views[0].size();
	}

	for(const auto& v : views) {
		if(v.size() != ref) {
			y_fatal("Invalid attachment size.");
		}
	}
	return ref;
}

static RenderPass* create_render_pass(DevicePtr dptr, const DepthAttachmentView& depth, core::ArrayView<ColorAttachmentView> colors) {
	auto color_vec = core::vector_with_capacity<RenderPass::ImageData>(colors.size());
	std::transform(colors.begin(), colors.end(), std::back_inserter(color_vec), [](const auto& c) { return RenderPass::ImageData(c); });
	return depth.device()
			? new RenderPass(dptr, depth, color_vec)
			: new RenderPass(dptr, color_vec);
}



Framebuffer::Framebuffer(DevicePtr dptr, core::ArrayView<ColorAttachmentView> colors) :
		Framebuffer(dptr, DepthAttachmentView(), colors) {
}

Framebuffer::Framebuffer(DevicePtr dptr, const DepthAttachmentView& depth, core::ArrayView<ColorAttachmentView> colors) :
		DeviceLinked(dptr),
		_size(compute_size(depth, colors)),
		_attachment_count(colors.size()),
		_render_pass(create_render_pass(dptr, depth, colors)),
		_depth(depth),
		_colors(colors.begin(), colors.end()) {

	auto views = core::vector_with_capacity<vk::ImageView>(_colors.size() + 1);
	std::transform(_colors.begin(), _colors.end(), std::back_inserter(views), [](const auto& v) { return v.vk_view(); });

	if(_depth.device()) {
		views << _depth.vk_view();
	}

	_framebuffer = device()->vk_device().createFramebuffer(vk::FramebufferCreateInfo()
		.setRenderPass(_render_pass->vk_render_pass())
		.setAttachmentCount(u32(views.size()))
		.setPAttachments(views.begin())
		.setWidth(_size.x())
		.setHeight(_size.y())
		.setLayers(1)
	);
}

Framebuffer::~Framebuffer() {
	destroy(_framebuffer);
}

Framebuffer::Framebuffer(Framebuffer&& other) {
	swap(other);
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) {
	swap(other);
	return *this;
}

void Framebuffer::swap(Framebuffer& other) {
	DeviceLinked::swap(other);
	std::swap(_size, other._size);
	std::swap(_attachment_count, other._attachment_count);
	std::swap(_render_pass, other._render_pass);
	std::swap(_framebuffer, other._framebuffer);
}

const math::Vec2ui& Framebuffer::size() const {
	return _size;
}

vk::Framebuffer Framebuffer::vk_framebuffer() const {
	return _framebuffer;
}

const RenderPass& Framebuffer::render_pass() const {
	return *_render_pass;
}

const DepthAttachmentView& Framebuffer::depth_attachment() const {
	return _depth;
}

const core::Vector<ColorAttachmentView>& Framebuffer::color_attachments() const {
	return _colors;
}

}
