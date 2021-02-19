/*******************************
Copyright (c) 2016-2021 Gr�goire Angerand

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

#include "vk.h"

namespace yave {

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

        default:
            break;
    }
    return "Unknown error";
#undef VK_RESULT_CASE
}

}

