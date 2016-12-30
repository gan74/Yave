/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

This code is licensed under the MIT License (MIT).

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
#ifndef YAVE_RENDERPASS_H
#define YAVE_RENDERPASS_H

#include "yave.h"

#include <yave/DeviceLinked.h>
#include <yave/image/ImageFormat.h>
#include <yave/image/ImageUsage.h>

namespace yave {

class RenderPass : NonCopyable, public DeviceLinked {
	public:
		RenderPass() = default;
		RenderPass(DevicePtr dptr, ImageFormat depth_format, std::initializer_list<ImageFormat> color_formats, ImageUsage color_usage = ImageUsage::Color | ImageUsage::Texture);

		RenderPass(RenderPass&& other);
		RenderPass& operator=(RenderPass&& other);

		~RenderPass();

		vk::RenderPass vk_render_pass() const;

		usize attachment_count() const {
			return _attachment_count;
		}

	private:
		void swap(RenderPass& other);

		usize _attachment_count = 0;
		vk::RenderPass _render_pass;
};

}

#endif // YAVE_RENDERPASS_H
