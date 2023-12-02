/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#ifndef VOLK_IMPLEMENTATION
#define VOLK_IMPLEMENTATION
#endif

#include "vk.h"

namespace yave {

void initialize_volk() {
    static bool volk_init = false;
    if(!volk_init) {
        vk_check(volkInitialize());
        volk_init = true;
    }
}

const char* vk_result_str(VkResult result) {
#define VK_RESULT_CASE(fmt) case fmt: return #fmt;
    switch(result) {
        VK_RESULT_CASE(VK_SUCCESS)
        VK_RESULT_CASE(VK_NOT_READY)
        VK_RESULT_CASE(VK_TIMEOUT)
        VK_RESULT_CASE(VK_EVENT_SET)
        VK_RESULT_CASE(VK_EVENT_RESET)
        VK_RESULT_CASE(VK_INCOMPLETE)
        VK_RESULT_CASE(VK_ERROR_OUT_OF_HOST_MEMORY)
        VK_RESULT_CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY)
        VK_RESULT_CASE(VK_ERROR_INITIALIZATION_FAILED)
        VK_RESULT_CASE(VK_ERROR_DEVICE_LOST)
        VK_RESULT_CASE(VK_ERROR_MEMORY_MAP_FAILED)
        VK_RESULT_CASE(VK_ERROR_LAYER_NOT_PRESENT)
        VK_RESULT_CASE(VK_ERROR_EXTENSION_NOT_PRESENT)
        VK_RESULT_CASE(VK_ERROR_FEATURE_NOT_PRESENT)
        VK_RESULT_CASE(VK_ERROR_INCOMPATIBLE_DRIVER)
        VK_RESULT_CASE(VK_ERROR_TOO_MANY_OBJECTS)
        VK_RESULT_CASE(VK_ERROR_FORMAT_NOT_SUPPORTED)
        VK_RESULT_CASE(VK_ERROR_FRAGMENTED_POOL)
        VK_RESULT_CASE(VK_ERROR_UNKNOWN)
        VK_RESULT_CASE(VK_ERROR_OUT_OF_POOL_MEMORY)
        VK_RESULT_CASE(VK_ERROR_INVALID_EXTERNAL_HANDLE)
        VK_RESULT_CASE(VK_ERROR_FRAGMENTATION)
        VK_RESULT_CASE(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)
        VK_RESULT_CASE(VK_PIPELINE_COMPILE_REQUIRED)
        VK_RESULT_CASE(VK_ERROR_SURFACE_LOST_KHR)
        VK_RESULT_CASE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
        VK_RESULT_CASE(VK_SUBOPTIMAL_KHR)
        VK_RESULT_CASE(VK_ERROR_OUT_OF_DATE_KHR)
        VK_RESULT_CASE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
        VK_RESULT_CASE(VK_ERROR_VALIDATION_FAILED_EXT)
        VK_RESULT_CASE(VK_ERROR_INVALID_SHADER_NV)
        VK_RESULT_CASE(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT)
        VK_RESULT_CASE(VK_ERROR_NOT_PERMITTED_KHR)
        VK_RESULT_CASE(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
        VK_RESULT_CASE(VK_THREAD_IDLE_KHR)
        VK_RESULT_CASE(VK_THREAD_DONE_KHR)
        VK_RESULT_CASE(VK_OPERATION_DEFERRED_KHR)
        VK_RESULT_CASE(VK_OPERATION_NOT_DEFERRED_KHR)

        default:
            break;
    }
    return "Unknown result";
#undef VK_RESULT_CASE
}

}

