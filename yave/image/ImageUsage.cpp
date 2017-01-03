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

#include "ImageUsage.h"

namespace yave {

static vk::ImageLayout vk_shader_layout(ImageUsage usage) {
	if((usage & ImageUsage::Swapchain) != ImageUsage::None) {
		return vk::ImageLayout::ePresentSrcKHR;
	}
	if((usage & ImageUsage::Texture) != ImageUsage::None) {
		return vk::ImageLayout::eShaderReadOnlyOptimal;
	}
	if((usage & ImageUsage::Storage) != ImageUsage::None) {
		return vk::ImageLayout::eShaderReadOnlyOptimal;
	}

	return fatal("Undefined image layout.");
}

vk::ImageLayout vk_attachment_image_layout(ImageUsage usage) {
	if((usage & ImageUsage::Depth) != ImageUsage::None) {
		return vk::ImageLayout::eDepthStencilAttachmentOptimal;
	}
	if((usage & ImageUsage::Color) != ImageUsage::None) {
		return vk::ImageLayout::eColorAttachmentOptimal;
	}

	return fatal("Undefined image layout.");
}

vk::ImageLayout vk_shader_image_layout(ImageUsage usage) {
	auto su = shader_usage(usage);
	return su == ImageUsage::None ? vk_attachment_image_layout(usage) : vk_shader_layout(su);
}

}
