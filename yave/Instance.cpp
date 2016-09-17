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

#include "Instance.h"


namespace yave {

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT, u64, usize, i32, const char* layer, const char* msg, void*) {
	log_msg(core::String() + "Vk: @[" + layer + "]: " + msg, flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ? LogType::Error : LogType::Info);
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
			.setEnabledExtensionCount(_instance_extentions.size())
			.setPpEnabledExtensionNames(_instance_extentions.begin())
			.setEnabledLayerCount(_debug_params.get_instance_layers().size())
			.setPpEnabledLayerNames(_debug_params.get_instance_layers().begin())
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
						vk::DebugReportFlagBitsEXT::eError |
						vk::DebugReportFlagBitsEXT::ePerformanceWarning |
						vk::DebugReportFlagBitsEXT::eWarning
					)
				;
			create_debug_callback(_instance,& create_info, nullptr,& _debug_callback);
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

const DebugParams& Instance::get_debug_params() const {
	return _debug_params;
}

vk::Instance Instance::get_vk_instance() const {
	return _instance;
}

}
