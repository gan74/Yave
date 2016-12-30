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

#include "ImageUsage.h"

namespace yave {

vk::ImageLayout vk_image_layout(ImageUsage usage) {
	if((usage & ImageUsage::Swapchain) == ImageUsage::Swapchain) {
		return vk::ImageLayout::ePresentSrcKHR;
	}

	if((usage & ImageUsage::Depth) == ImageUsage::Depth) {
		return vk::ImageLayout::eDepthStencilAttachmentOptimal;
	}
	if((usage & ImageUsage::Texture) == ImageUsage::Texture) {
		return vk::ImageLayout::eShaderReadOnlyOptimal;
	}
	if((usage & ImageUsage::Color) == ImageUsage::Color) {
		return vk::ImageLayout::eColorAttachmentOptimal;
	}

	return vk::ImageLayout::eUndefined;
}

vk::AccessFlags vk_access_flags(ImageUsage usage) {
	/*
	 *
	 * This WILL cause some debug layers to complain because of additional bits sets.
	 * This is NORMAL (but not intended) when creating texture images that are not used as textures.
	 *
	 */

	auto flags = vk::AccessFlags();
	flags = (usage & ImageUsage::Depth) == ImageUsage::Depth ?
		flags | vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite :
		flags;

	flags = (usage & ImageUsage::Color) == ImageUsage::Color ?
		flags | vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite :
		flags;

	flags = (usage & ImageUsage::Texture) == ImageUsage::Texture ?
		flags | vk::AccessFlagBits::eShaderRead :
		flags;

	return flags;
}

}
