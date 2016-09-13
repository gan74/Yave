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

#include "LowLevelGraphics.h"
#include "RenderPass.h"

namespace yave {

math::Vec2ui compute_size(const ImageBase& a, const ImageBase& b) {
	if(a.size() != b.size()) {
		fatal("Invalid attachment size");
	}
	return a.size();
}

Framebuffer::Framebuffer(DevicePtr dptr, RenderPass& render_pass, DepthAttachmentView depth, ColorAttachmentView color) :
		DeviceLinked(dptr),
		_size(compute_size(depth.get_image(), color.get_image())),
		_render_pass(&render_pass) {

	auto views = {color.get_vk_image_view(), depth.get_vk_image_view()};

	_framebuffer = dptr->get_vk_device().createFramebuffer(vk::FramebufferCreateInfo()
		.setRenderPass(render_pass.get_vk_render_pass())
		.setAttachmentCount(views.size())
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
	std::swap(_render_pass, other._render_pass);
	std::swap(_framebuffer, other._framebuffer);
}

const math::Vec2ui &Framebuffer::size() const {
	return _size;
}

const RenderPass &Framebuffer::get_render_pass() const {
	return *_render_pass;
}

vk::Framebuffer Framebuffer::get_vk_framebuffer() const {
	return _framebuffer;
}

}
