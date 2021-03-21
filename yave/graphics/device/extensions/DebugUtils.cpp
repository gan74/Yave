/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include <yave/graphics/graphics.h>

#include <y/core/String.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_message_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void*) {


    {
        constexpr std::string_view black_listed = "UNASSIGNED-CoreValidation-DrawState-VtxIndexOutOfBounds";
        if(black_listed == callback_data->pMessageIdName) {
            return false;
        }
    }

    Log level = Log::Debug;
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

    core::String labels = "";
    for(usize i = 0; i != callback_data->cmdBufLabelCount; ++i) {
        labels += callback_data->pCmdBufLabels[callback_data->cmdBufLabelCount - i - 1].pLabelName;
        labels += " > ";
    }

    log_msg(fmt("Vk: @[%] %:\n%\n", src, labels, callback_data->pMessage),  level);

    if(severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        y_breakpoint;
    }

    return false;
}



const char* DebugUtils::extension_name() {
    return VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
}

DebugUtils::DebugUtils(VkInstance instance) : _instance(instance) {

    _begin_label = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(_instance, "vkCmdBeginDebugUtilsLabelEXT"));
    _end_label = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(_instance, "vkCmdEndDebugUtilsLabelEXT"));
    _set_object_name = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(_instance, "vkSetDebugUtilsObjectNameEXT"));

    const auto create_messenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT"));

    VkDebugUtilsMessengerCreateInfoEXT create_info = vk_struct();
    {
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
    }

    Y_TODO(Hook allocation callbacks)
    vk_check(create_messenger(_instance, &create_info, nullptr, &_messenger.get()));
}

DebugUtils::~DebugUtils() {
    const auto destroy_messenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT"));
    destroy_messenger(_instance, _messenger, nullptr);
}

void DebugUtils::begin_region(VkCommandBuffer buffer, const char* name, const math::Vec4& color) const {
    VkDebugUtilsLabelEXT label = vk_struct();
    label.pLabelName = name;
    for(usize i = 0; i != 4; ++i) {
        label.color[i] = color[i];
    }

    _begin_label(buffer, &label);
}

void DebugUtils::end_region(VkCommandBuffer buffer) const {
    _end_label(buffer);
}

void DebugUtils::set_name(u64 resource, VkObjectType type, const char *name) const {
    VkDebugUtilsObjectNameInfoEXT name_info = vk_struct();
    name_info.objectType = type;
    name_info.objectHandle = resource;
    name_info.pObjectName = name;

    y_debug_assert(_set_object_name);
    vk_check(_set_object_name(vk_device(), &name_info));
}

}

