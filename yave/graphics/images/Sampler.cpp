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

#include "Sampler.h"

#include <yave/device/DeviceUtils.h>

namespace yave {

static VkSamplerAddressMode vk_address_mode(SamplerType type) {
    switch(type) {
        case SamplerType::Repeat:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;

        case SamplerType::Clamp:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        default:
            y_fatal("Unknown sampler type");
    }
}

static VkSampler create_sampler(DevicePtr dptr, SamplerType type) {

    VkSamplerCreateInfo create_info = vk_struct();
    {
        const VkSamplerAddressMode address_mode = vk_address_mode(type);

        create_info.addressModeU = address_mode;
        create_info.addressModeV = address_mode;
        create_info.addressModeW = address_mode;
        create_info.magFilter = VK_FILTER_LINEAR;
        create_info.minFilter = VK_FILTER_LINEAR;
        create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        create_info.maxLod = 1000.0f;
        create_info.maxAnisotropy = 1.0f;
    }

    VkSampler sampler = {};
    vk_check(vkCreateSampler(vk_device(dptr), &create_info, vk_allocation_callbacks(dptr), &sampler));
    return sampler;
}

Sampler::Sampler(DevicePtr dptr, SamplerType type) : DeviceLinked(dptr), _sampler(create_sampler(dptr, type)) {
}

Sampler::~Sampler() {
    destroy(_sampler);
}

VkSampler Sampler::vk_sampler() const {
    return _sampler;
}


}

