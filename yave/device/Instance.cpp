/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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

Instance::Instance(DebugParams debug) : _debug_params(debug) {
	auto extention_names = core::vector_with_capacity<const char*>(4);
	extention_names = {VK_KHR_SURFACE_EXTENSION_NAME};

	if(_debug_params.debug_features_enabled()) {
		extention_names << DebugCallback::name();
	}


	#ifdef Y_OS_WIN
		extention_names << VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
	#endif

	auto app_info = vk::ApplicationInfo()
			.setApiVersion(VK_MAKE_VERSION(1, 0, 0))
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
		_extensions.debug_callback = new DebugCallback(_instance);
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

vk::Instance Instance::vk_instance() const {
	return _instance;
}

}
