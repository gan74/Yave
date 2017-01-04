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
#ifndef YAVE_FRAMEBUFFER_H
#define YAVE_FRAMEBUFFER_H

#include "yave.h"

#include "RenderPass.h"

#include <yave/image/ImageView.h>

namespace yave {

class Framebuffer : NonCopyable, public DeviceLinked {

	public:
		Framebuffer() = default;

		Framebuffer(RenderPass& render_pass, DepthAttachmentView depth, std::initializer_list<ColorAttachmentView> colors);
		Framebuffer(DevicePtr dptr, DepthAttachmentView depth, std::initializer_list<ColorAttachmentView> colors);

		Framebuffer(Framebuffer&& other);
		Framebuffer& operator=(Framebuffer&& other);

		~Framebuffer();

		const math::Vec2ui& size() const;

		vk::Framebuffer vk_framebuffer() const;

		const DepthAttachmentView& depth_attachment() const;
		const core::Vector<ColorAttachmentView>& color_attachments() const;
		const RenderPass& render_pass() const;

		usize attachment_count() const {
			return _attachment_count;
		}

	private:
		Framebuffer(DevicePtr dptr, const RenderPass* render_pass, const DepthAttachmentView& depth, std::initializer_list<ColorAttachmentView> colors);

		void swap(Framebuffer& other);

		core::Ptr<RenderPass> _render_pass_storage;

		math::Vec2ui _size;
		usize _attachment_count = 0;

		const RenderPass* _render_pass = nullptr;
		vk::Framebuffer _framebuffer;

		DepthAttachmentView _depth;
		core::Vector<ColorAttachmentView> _colors;
};

}

#endif // YAVE_FRAMEBUFFER_H
