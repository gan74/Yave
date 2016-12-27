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
