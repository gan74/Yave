/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#include "DebugUtils.h"

#include "deviceutils.h"

#include <yave/graphics/graphics.h>

#include <y/core/Span.h>
#include <y/core/Vector.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

static bool try_enable_extension(core::Vector<const char*>& exts, const char* name) {
    core::Vector<VkExtensionProperties> supported_extensions;
    {
        u32 count = 0;
        vk_check(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
        supported_extensions = core::Vector<VkExtensionProperties>(count, VkExtensionProperties{});
        vk_check(vkEnumerateInstanceExtensionProperties(nullptr, &count, supported_extensions.data()));
    }

    const std::string_view name_view = name;
    for(const VkExtensionProperties& ext : supported_extensions) {
        if(name_view == ext.extensionName) {
            exts << name;
            return true;
        }
    }
    log_msg(fmt("{} not supported", name_view), Log::Warning);
    return false;
}

Instance::Instance(const InstanceParams& params) : _params(params) {
    _params.debug_utils |= _params.validation_layers;

    initialize_volk();

    auto extension_names = core::Vector<const char*>::with_capacity(8);
    extension_names = {
        VK_KHR_SURFACE_EXTENSION_NAME,
    };

#ifdef Y_OS_WIN
    extension_names << VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#endif

#ifdef Y_OS_LINUX
    extension_names << VK_KHR_XCB_SURFACE_EXTENSION_NAME;
#endif

    if(_params.debug_utils) {
        _params.debug_utils = try_enable_extension(extension_names, DebugUtils::extension_name());
    }

    VkApplicationInfo app_info = vk_struct();
    {
        app_info.apiVersion = VK_API_VERSION_1_3;
        app_info.pApplicationName = "Yave";
        app_info.pEngineName = "Yave";
    }

    VkInstanceCreateInfo create_info = vk_struct();
    {
        create_info.enabledExtensionCount = u32(extension_names.size());
        create_info.ppEnabledExtensionNames = extension_names.data();
        create_info.pApplicationInfo = &app_info;
    }

    if(_params.validation_layers) {
        log_msg("Vulkan validation enabled");
        const auto ext = validation_extensions();
        create_info.enabledLayerCount = u32(ext.size());
        create_info.ppEnabledLayerNames = ext.data();
    }

    vk_check(vkCreateInstance(&create_info, vk_allocation_callbacks(), &_instance));

    volkLoadInstance(_instance);

    if(_params.debug_utils) {
        log_msg("Vulkan debug utils enabled");
        _extensions.debug_utils = std::make_unique<DebugUtils>(_instance);
    }
}

Instance::~Instance() {
    // destroy extensions to free everything before the instance gets destroyed
    _extensions = {};
    vkDestroyInstance(_instance, vk_allocation_callbacks());
}

const InstanceParams& Instance::instance_params() const {
    return _params;
}

const DebugUtils* Instance::debug_utils() const {
    return _extensions.debug_utils.get();
}

VkInstance Instance::vk_instance() const {
    return _instance;
}

core::Vector<PhysicalDevice> Instance::physical_devices() const {

    core::Vector<VkPhysicalDevice> vk_devices;
    {
        u32 count = 0;
        vk_check(vkEnumeratePhysicalDevices(_instance, &count, nullptr));
        vk_devices = core::Vector<VkPhysicalDevice>(count, VkPhysicalDevice{});
        vk_check(vkEnumeratePhysicalDevices(_instance, &count, vk_devices.data()));
    }

    auto devices = core::Vector<PhysicalDevice>::with_capacity(vk_devices.size());
    std::transform(vk_devices.begin(), vk_devices.end(), std::back_inserter(devices), [](VkPhysicalDevice d) { return PhysicalDevice(d); });
    return devices;
}

}

