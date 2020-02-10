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
#define YAVE_VK_PLATFORM_INCLUDES

#include "Instance.h"
#include "extentions/DebugUtils.h"

#include <y/utils/log.h>

namespace yave {

Instance::Instance(DebugParams debug) : _debug_params(debug) {
	auto extention_names = core::vector_with_capacity<const char*>(4);
	extention_names = {VK_KHR_SURFACE_EXTENSION_NAME};

	if(_debug_params.debug_features_enabled()) {
		//extention_names << DebugCallback::name();
		extention_names << DebugUtils::name();
	}

	#ifdef Y_OS_WIN
		extention_names << VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
	#endif

	const auto app_info = vk::ApplicationInfo()
			.setApiVersion(VK_MAKE_VERSION(1, 2, 0))
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
		//_extensions.debug_callback = std::make_unique<DebugCallback>(_instance);
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
