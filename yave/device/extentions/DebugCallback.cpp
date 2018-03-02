/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include "DebugCallback.h"

#include <yave/device/Instance.h>
#include <yave/vk/vk.h>

namespace yave {

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT, u64, usize, i32, const char* layer, const char* msg, void*) {
	auto type = Log::Info;
	if(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		// breakpoint here
		type = Log::Error;
	}
	log_msg(core::String() + "Vk: @[" + layer + "]: " + msg + "\n",  type);
	return false;
}



const char* DebugCallback::name() {
	return VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
}

DebugCallback::DebugCallback(vk::Instance instance) : _instance(instance) {
	vk::DebugReportCallbackCreateInfoEXT create_info = vk::DebugReportCallbackCreateInfoEXT()
			.setPfnCallback(vulkan_debug)
			.setFlags(
				//vk::DebugReportFlagBitsEXT::eDebug |
				//vk::DebugReportFlagBitsEXT::eInformation |
				vk::DebugReportFlagBitsEXT::eError |
				vk::DebugReportFlagBitsEXT::ePerformanceWarning |
				vk::DebugReportFlagBitsEXT::eWarning
			)
		;

	auto create_callback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(_instance.getProcAddr("vkCreateDebugReportCallbackEXT"));
	create_callback(_instance, reinterpret_cast<const VkDebugReportCallbackCreateInfoEXT*>(&create_info), nullptr, &_callback);
}

DebugCallback::~DebugCallback() {
	auto destroy_callback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(_instance.getProcAddr("vkDestroyDebugReportCallbackEXT"));
	destroy_callback(_instance, _callback, nullptr);
}

}
