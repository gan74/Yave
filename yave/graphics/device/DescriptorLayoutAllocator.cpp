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

#include "DescriptorLayoutAllocator.h"

#include <yave/graphics/graphics.h>

namespace yave {

DescriptorLayoutAllocator::~DescriptorLayoutAllocator() {
    _layouts.locked([](auto&& layouts) {
        for(auto& [k, l] : layouts) {
            destroy_graphic_resource(std::move(l));
        }
        layouts.clear();
    });
}

VkDescriptorSetLayout DescriptorLayoutAllocator::create_layout(core::Span<VkDescriptorSetLayoutBinding> bindings) {
    return _layouts.locked([&](auto&& layouts) {
        auto& layout = layouts[bindings];
        if(!layout) {
            VkDescriptorSetLayoutCreateInfo create_info = vk_struct();
            {
                create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
                create_info.bindingCount = u32(bindings.size());
                create_info.pBindings = bindings.data();
            }

            vk_check(vkCreateDescriptorSetLayout(vk_device(), &create_info, vk_allocation_callbacks(), layout.get_ptr_for_init()));
        }

        return layout.get();
    });
}

usize DescriptorLayoutAllocator::layout_count() const {
    return _layouts.locked([](const auto& layouts) { return layouts.size(); });
}

}
