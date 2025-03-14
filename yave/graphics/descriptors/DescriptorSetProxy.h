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
#ifndef YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORSETPROXY_H
#define YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORSETPROXY_H

#include "Descriptor.h"


namespace yave {

class DescriptorSetProxy final : NonMovable {
    public:
        DescriptorSetProxy() = default;

        DescriptorSetProxy(VkDescriptorSet set) : _set(set) {
        }

        DescriptorSetProxy(core::Span<Descriptor> descriptors) : _descriptors(descriptors) {
        }

        DescriptorSetProxy(const Descriptor& descriptor) : DescriptorSetProxy(core::Span(descriptor)) {
        }

        template<usize N>
        DescriptorSetProxy(const std::array<Descriptor, N>& descriptors) : _descriptors(core::Span<Descriptor>(descriptors)) {
        }

        VkDescriptorSet vk_descriptor_set() const {
            return _set;
        }

        core::Span<Descriptor> descriptors() const {
            return _descriptors;
        }

    private:
        core::Span<Descriptor> _descriptors;
        VkDescriptorSet _set = {};
};

template<typename... Args>
auto make_descriptor_set(Args&&... args) {
    return std::array<Descriptor, sizeof...(args)>{
        Descriptor(y_fwd(args))...
    };
}

}

#endif // YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORSETPROXY_H

