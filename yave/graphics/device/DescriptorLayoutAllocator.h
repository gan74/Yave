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
#ifndef YAVE_GRAPHICS_DEVICE_DESCRIPTORLAYOUTALLOCATOR_H
#define YAVE_GRAPHICS_DEVICE_DESCRIPTORLAYOUTALLOCATOR_H

#include <yave/yave.h>

#include <y/concurrent/Mutexed.h>

#include <y/core/Span.h>
#include <y/core/Vector.h>
#include <y/core/HashMap.h>

#include <y/utils/hash.h>

#include <yave/graphics/vk/vk.h>

namespace yave {

class DescriptorLayoutAllocator : NonMovable {
    public:
        using LayoutKey = core::Span<VkDescriptorSetLayoutBinding>;

        ~DescriptorLayoutAllocator();

        VkDescriptorSetLayout create_layout(core::Span<VkDescriptorSetLayoutBinding> bindings);

        usize layout_count() const;

    private:
        struct KeyEqual {
            inline bool operator()(LayoutKey a, LayoutKey b) const {
                return a == b;
            }

            inline bool operator()(const core::Vector<VkDescriptorSetLayoutBinding>& a, LayoutKey b) const {
                return LayoutKey(a) == b;
            }
        };

        struct KeyHash {
            inline auto operator()(LayoutKey k) const {
                usize h = 0;
                static_assert(sizeof(VkDescriptorSetLayoutBinding) % sizeof(u32) == 0);
                for(const VkDescriptorSetLayoutBinding& l : k) {
                    const usize lh = hash_range(core::Span<u32>(reinterpret_cast<const u32*>(&l), sizeof(VkDescriptorSetLayoutBinding) / sizeof(u32)));
                    hash_combine(h, lh);
                }
                return h;
            }

            inline auto operator()(const core::Vector<VkDescriptorSetLayoutBinding>& k) const {
                return operator()(LayoutKey(k));
            }
        };


        concurrent::Mutexed<core::FlatHashMap<core::Vector<VkDescriptorSetLayoutBinding>, VkHandle<VkDescriptorSetLayout>, KeyHash, KeyEqual>> _layouts;
};

}

#endif // YAVE_GRAPHICS_DEVICE_DESCRIPTORLAYOUTALLOCATOR_H

