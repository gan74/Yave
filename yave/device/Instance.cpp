/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#include "extentions/DebugUtils.h"

#include <y/core/Span.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

static bool try_enable_extension(core::Vector<const char*>& exts, const char* name) {
	const std::string_view name_view = name;
	const auto supported_extensions = vk::enumerateInstanceExtensionProperties();
	for(vk::ExtensionProperties ext : supported_extensions) {
		if(name_view == ext.extensionName) {
			exts << name;
			return true;
		}
	}
	log_msg(fmt("% not supported", name_view), Log::Warning);
	return false;
}

Instance::Instance(DebugParams debug) : _debug_params(debug) {
	auto extention_names = core::vector_with_capacity<const char*>(4);
	extention_names = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
	};

#ifdef Y_OS_WIN
	extention_names << VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#endif

	if(_debug_params.debug_features_enabled()) {
		_debug_params.set_enabled(try_enable_extension(extention_names, DebugUtils::name()));
	}


	const auto app_info = vk::ApplicationInfo()
			.setApiVersion(VK_VERSION_1_1)
			.setPApplicationName("Yave")
			.setPEngineName("Yave")
		;

	_instance = vk::createInstance(vk::InstanceCreateInfo()
			.setEnabledExtensionCount(u32(extention_names.size()))
			.setPpEnabledExtensionNames(extention_names.begin())
			.setEnabledLayerCount(u32(_debug_params.instance_layers().size()))
			.setPpEnabledLayerNames(_debug_params.instance_layers().begin())
			.setPApplicationInfo(&app_info)
		);

	if(_debug_params.debug_features_enabled()) {
		log_msg("Vulkan debugging enabled.");
		_extensions.debug_utils = std::make_unique<DebugUtils>(_instance);
	}
}

Instance::~Instance() {
	// destroy extensions to free everything before the instance gets destroyed
	_extensions = {};
	_instance.destroy();
}

const DebugParams& Instance::debug_params() const {
	return _debug_params;
}

const DebugUtils* Instance::debug_utils() const {
	return _extensions.debug_utils.get();
}

vk::Instance Instance::vk_instance() const {
	return _instance;
}

}
