/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include "DebugUtils.h"

#include <yave/device/Device.h>

namespace yave {

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_message_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
		void*) {

	Log level = Log::Info;
	switch(severity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			level = Log::Info;
		break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			level = Log::Warning;
		break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			level = Log::Error;
		break;

		default:
		break;
	}

	std::string_view src = "VK";
	if(type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
		src = "Validation";
	}
	if(type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
		src = "Performance";
	}

	log_msg(fmt("Vk: @[%]: %\n", src, callback_data->pMessage),  level);

	return false;
}



const char* DebugUtils::name() {
	return VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
}

DebugUtils::DebugUtils(vk::Instance instance) : _instance(instance) {

	_begin_label = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(_instance.getProcAddr("vkCmdBeginDebugUtilsLabelEXT"));
	_end_label = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(_instance.getProcAddr("vkCmdEndDebugUtilsLabelEXT"));


	auto create_messenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(_instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));

	VkDebugUtilsMessengerCreateInfoEXT create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
		;
	create_info.messageSeverity =
			//VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			//VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
		;
	create_info.pfnUserCallback = vulkan_message_callback;

	create_messenger(_instance, &create_info, nullptr, &_messenger);
}

DebugUtils::~DebugUtils() {
	auto destroy_messenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(_instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
	destroy_messenger(_instance, _messenger, nullptr);
}

void DebugUtils::begin_region(vk::CommandBuffer buffer, const char* name, const math::Vec4& color) const {
	VkDebugUtilsLabelEXT label = {};
	label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label.pLabelName = name;
	for(usize i = 0; i != 4; ++i) {
		label.color[i] = color[i];
	}

	_begin_label(buffer, &label);
}

void DebugUtils::end_region(vk::CommandBuffer buffer) const {
	_end_label(buffer);
}

void DebugUtils::set_resource_name(DevicePtr dptr, u64 resource, const char *name) const {
	VkDebugUtilsObjectNameInfoEXT name_info = {};
	name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	name_info.objectType = VK_OBJECT_TYPE_UNKNOWN;
	name_info.objectHandle = resource;
	name_info.pObjectName = name;

	_set_object_name(dptr->vk_device(), &name_info);
}

}
