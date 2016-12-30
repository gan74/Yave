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

#include "Instance.h"


namespace yave {

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT, u64, usize, i32, const char* layer, const char* msg, void*) {
	log_msg(core::String() + "Vk: @[" + layer + "]: " + msg + "\n", flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ? LogType::Error : LogType::Info);
	return false;
}


Instance::Instance(DebugParams debug) : _debug_params(debug), _debug_callback(nullptr) {
	_instance_extentions	= {VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_KHR_SURFACE_EXTENSION_NAME};

	#ifdef Y_OS_WIN
		_instance_extentions.append(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	#endif

	auto app_info = vk::ApplicationInfo()
			.setApiVersion(VK_MAKE_VERSION(1, 0, 0))
			.setPApplicationName("Yave")
			.setPEngineName("Yave")
		;

	_instance = vk::createInstance(vk::InstanceCreateInfo()
			.setEnabledExtensionCount(u32(_instance_extentions.size()))
			.setPpEnabledExtensionNames(_instance_extentions.begin())
			.setEnabledLayerCount(u32(_debug_params.instance_layers().size()))
			.setPpEnabledLayerNames(_debug_params.instance_layers().begin())
			.setPApplicationInfo(&app_info)
		);

	setup_debug();
}

void Instance::setup_debug() {
	if(_debug_params.is_debug_callback_enabled()) {
		auto create_debug_callback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(_instance.getProcAddr("vkCreateDebugReportCallbackEXT"));
		if(create_debug_callback) {
			VkDebugReportCallbackCreateInfoEXT create_info = vk::DebugReportCallbackCreateInfoEXT()
					.setPfnCallback(vulkan_debug)
					.setFlags(
						//vk::DebugReportFlagBitsEXT::eDebug |
						//vk::DebugReportFlagBitsEXT::eInformation |
						vk::DebugReportFlagBitsEXT::eError |
						vk::DebugReportFlagBitsEXT::ePerformanceWarning |
						vk::DebugReportFlagBitsEXT::eWarning
					)
				;
			create_debug_callback(_instance, &create_info, nullptr, &_debug_callback);
		}
	}
}

Instance::~Instance() {
	if(_debug_callback) {
		auto destroy_debug_callback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(_instance.getProcAddr("vkDestroyDebugReportCallbackEXT"));
		if(destroy_debug_callback) {
			destroy_debug_callback(_instance, _debug_callback, nullptr);
		}
	}
	_instance.destroy();
}

const DebugParams& Instance::debug_params() const {
	return _debug_params;
}

vk::Instance Instance::vk_instance() const {
	return _instance;
}

}
