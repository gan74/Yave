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

#include "Sampler.h"

#include <yave/graphics/graphics.h>

namespace yave {

static VkSamplerAddressMode vk_address_mode(SamplerType type) {
    switch(type) {
        case SamplerType::LinearRepeat:
        case SamplerType::PointRepeat:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;

        case SamplerType::Shadow:
        case SamplerType::LinearClamp:
        case SamplerType::PointClamp:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        default:
            y_fatal("Unknown sampler type");
    }
}

static VkFilter vk_filter(SamplerType type) {
    switch(type) {
        case SamplerType::PointRepeat:
        case SamplerType::PointClamp:
            return VK_FILTER_NEAREST;

        case SamplerType::Shadow:
        case SamplerType::LinearRepeat:
        case SamplerType::LinearClamp:
            return VK_FILTER_LINEAR;

        default:
            y_fatal("Unknown sampler type");
    }
}

static VkSamplerMipmapMode vk_mip_filter(SamplerType type) {
    switch(type) {
        case SamplerType::PointRepeat:
        case SamplerType::PointClamp:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case SamplerType::Shadow:
        case SamplerType::LinearRepeat:
        case SamplerType::LinearClamp:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;

        default:
            y_fatal("Unknown sampler type");
    }
}


static VkSampler create_sampler(SamplerType type) {

    VkSamplerCreateInfo create_info = vk_struct();
    {
        const VkSamplerAddressMode address_mode = vk_address_mode(type);
        const VkFilter filter = vk_filter(type);
        const VkSamplerMipmapMode mip_filter = vk_mip_filter(type);

        create_info.addressModeU = address_mode;
        create_info.addressModeV = address_mode;
        create_info.addressModeW = address_mode;
        create_info.magFilter = filter;
        create_info.minFilter = filter;
        create_info.mipmapMode = mip_filter;
        create_info.maxLod = 1000.0f;
        create_info.maxAnisotropy = 1.0f;
        create_info.compareEnable = type == SamplerType::Shadow;
        create_info.compareOp = VK_COMPARE_OP_GREATER;
    }

    VkSampler sampler = {};
    vk_check(vkCreateSampler(vk_device(), &create_info, vk_allocation_callbacks(), &sampler));
    return sampler;
}

Sampler::Sampler(SamplerType type) : _sampler(create_sampler(type)) {
}

Sampler::~Sampler() {
    device_destroy(_sampler);
}

VkSampler Sampler::vk_sampler() const {
    return _sampler;
}


}

