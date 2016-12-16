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


vk::ImageUsageFlags ImageUsage::vk_image_usage() const {
	return _usage;
}

vk::ImageLayout ImageUsage::vk_image_layout() const{
	auto swapchain_bit = vk::ImageUsageFlagBits(ImageUsageBits::SwapchainBit);
	if(_usage & swapchain_bit) {
		return vk::ImageLayout::ePresentSrcKHR;
	}

	if(_usage & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
		return vk::ImageLayout::eDepthStencilAttachmentOptimal;
	}
	if(_usage & vk::ImageUsageFlagBits::eSampled) {
		return vk::ImageLayout::eShaderReadOnlyOptimal;
	}
	if(_usage & vk::ImageUsageFlagBits::eColorAttachment) {
		return vk::ImageLayout::eColorAttachmentOptimal;
	}

	return vk::ImageLayout::eUndefined;
}

vk::AccessFlags ImageUsage::vk_access_flags() const {
	/*
	 *
	 * This WILL cause some debug layers to complain because of additional bits sets.
	 * This is NORMAL (but not intended) when creating texture images that are not used as textures.
	 *
	 */

	auto flags = vk::AccessFlags();
	flags = (_usage & vk::ImageUsageFlagBits::eDepthStencilAttachment) ?
		flags | vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite :
		flags;

	flags = (_usage & vk::ImageUsageFlagBits::eColorAttachment) ?
		flags | vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite :
		flags;

	flags = (_usage & vk::ImageUsageFlagBits::eSampled) ?
		flags | vk::AccessFlagBits::eShaderRead :
		flags;

	return flags;
}

}
