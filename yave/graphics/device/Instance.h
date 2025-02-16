/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#ifndef YAVE_DEVICE_INSTANCE_H
#define YAVE_DEVICE_INSTANCE_H

#include "PhysicalDevice.h"

#include <y/core/Vector.h>

#include <memory>

namespace yave {

class DebugUtils;

struct InstanceParams {
    bool validation_layers = false;
    bool debug_utils = true;
    bool raytracing = true;

};

class Instance : NonMovable {
    public:
        Instance(const InstanceParams& params);
        ~Instance();

        const InstanceParams& instance_params() const;

        const DebugUtils* debug_utils() const;

        VkInstance vk_instance() const;

        core::Vector<PhysicalDevice> physical_devices() const;

    private:
        struct {
            std::unique_ptr<DebugUtils> debug_utils;
        } _extensions;

        VkInstance _instance = {};

        InstanceParams _params;

};

}

#endif // YAVE_DEVICE_INSTANCE_H

