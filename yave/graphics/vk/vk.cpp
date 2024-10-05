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

#ifndef VOLK_IMPLEMENTATION
#define VOLK_IMPLEMENTATION
#endif

#include "vk.h"

#include <vulkan/vk_enum_string_helper.h>

#include <yave/graphics/graphics.h>
#include <yave/graphics/device/DiagnosticCheckpoints.h>

namespace yave {

void initialize_volk() {
    static bool volk_init = false;
    if(!volk_init) {
        vk_check(volkInitialize());
        volk_init = true;
    }
}

const char* vk_result_str(VkResult result) {
    return string_VkResult(result);
}

void on_vk_device_lost() {
    if(const auto* diag = diagnostic_checkpoints()) {
        diag->dump_checkpoints();
    }
}

}

