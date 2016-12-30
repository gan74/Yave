/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#include "Device.h"

namespace yave {

static math::Vec2ui compute_size(const ImageBase& a, std::initializer_list<ColorAttachmentView> views) {
	for(const auto& v : views) {
		if(v.image().size() != a.size()) {
			fatal("Invalid attachment size");
		}
	}
	return a.size();
}


Framebuffer::Framebuffer(RenderPass& render_pass, DepthAttachmentView depth, std::initializer_list<ColorAttachmentView> colors) :
		_size(compute_size(depth.image(), colors)),
		_attachment_count(colors.size()),
		_render_pass(&render_pass) {

	auto views = core::range(colors).map([](const auto& v) { return v.vk_image_view(); }).collect<core::Vector>() + depth.vk_image_view();

	_framebuffer = device()->vk_device().createFramebuffer(vk::FramebufferCreateInfo()
		.setRenderPass(render_pass.vk_render_pass())
		.setAttachmentCount(u32(views.size()))
		.setPAttachments(views.begin())
		.setWidth(_size.x())
		.setHeight(_size.y())
		.setLayers(1)
	);
}

Framebuffer::~Framebuffer() {
	if(_render_pass) {
		device()->destroy(_framebuffer);
	}
}

Framebuffer::Framebuffer(Framebuffer&& other) : Framebuffer() {
	swap(other);
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) {
	swap(other);
	return *this;
}

void Framebuffer::swap(Framebuffer& other) {
	std::swap(_size, other._size);
	std::swap(_attachment_count, other._attachment_count);
	std::swap(_render_pass, other._render_pass);
	std::swap(_framebuffer, other._framebuffer);
}

const math::Vec2ui& Framebuffer::size() const {
	return _size;
}

const RenderPass& Framebuffer::render_pass() const {
	return *_render_pass;
}

vk::Framebuffer Framebuffer::vk_framebuffer() const {
	return _framebuffer;
}

DevicePtr Framebuffer::device() const {
	return _render_pass->device();
}

}
