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
