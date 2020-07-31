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
#ifndef YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORSETALLOCATOR_H
#define YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORSETALLOCATOR_H

#include <yave/graphics/vk/vk.h>
#include <yave/graphics/buffers/Buffer.h>
#include <yave/graphics/device/DeviceLinked.h>

#include <y/utils/hash.h>
#include <y/core/Vector.h>
#include <y/concurrent/SpinLock.h>

#include <mutex>
#include <bitset>
#include <memory>
#include <algorithm>
#include <memory>

namespace std {
template<>
struct hash<VkDescriptorSetLayoutBinding> {
    auto operator()(const VkDescriptorSetLayoutBinding& l) const {
        const char* data = reinterpret_cast<const char*>(&l);
        return y::hash_range(data, data + sizeof(l));
    }
};

template<>
struct hash<y::core::Vector<VkDescriptorSetLayoutBinding>> {
    auto operator()(const y::core::Vector<VkDescriptorSetLayoutBinding>& k) const {
        return y::hash_range(k);
    }
};
}

namespace yave {

class Descriptor;
class DescriptorSetPool;

class DescriptorSetLayout : NonCopyable, public DeviceLinked {
    public:
        static constexpr usize descriptor_type_count = 12;

        struct InlineBlock {
            u32 binding;
            u32 byte_size;
        };

        DescriptorSetLayout() = default;
        DescriptorSetLayout(DevicePtr dptr, core::Span<VkDescriptorSetLayoutBinding> bindings);

        DescriptorSetLayout(DescriptorSetLayout&&) = default;
        DescriptorSetLayout& operator=(DescriptorSetLayout&&) = default;

        ~DescriptorSetLayout();

        const std::array<u32, descriptor_type_count>& desciptors_count() const;

        core::Span<InlineBlock> inline_blocks_fallbacks() const;
        usize inline_blocks() const;

        VkDescriptorSetLayout vk_descriptor_set_layout() const;

    private:
        SwapMove<VkDescriptorSetLayout> _layout;
        std::array<u32, descriptor_type_count> _sizes = {};

        usize _inline_blocks = 0;
        core::Vector<InlineBlock> _inline_blocks_fallbacks;
};

class DescriptorSetData {
    public:
        DescriptorSetData() = default;

        DevicePtr device() const;
        bool is_null() const;

        VkDescriptorSetLayout vk_descriptor_set_layout() const;
        VkDescriptorSet vk_descriptor_set() const;

    private:
        friend class LifetimeManager;

        void recycle();

    private:
        friend class DescriptorSetPool;

        DescriptorSetData(DescriptorSetPool* pool, u32 id);

        DescriptorSetPool* _pool = nullptr;
        u32 _index = 0;
};

class DescriptorSetPool : NonMovable, public DeviceLinked {
    public:
        static constexpr usize pool_size = 128;

        DescriptorSetPool(const DescriptorSetLayout& layout);
        ~DescriptorSetPool();

        DescriptorSetData alloc(core::Span<Descriptor> descriptors);
        void recycle(u32 id);

        bool is_full() const;

        VkDescriptorSet vk_descriptor_set(u32 id) const;
        VkDescriptorPool vk_pool() const;
        VkDescriptorSetLayout vk_descriptor_set_layout() const;

        // Slow: for debug only
        usize free_sets() const;
        usize used_sets() const;

    private:
        void update_set(u32 id, core::Span<Descriptor> descriptors);

        usize inline_sub_buffer_alignment() const;

        std::bitset<pool_size> _taken;
        u32 _first_free = 0;

        mutable concurrent::SpinLock _lock;

        std::array<VkDescriptorSet, pool_size> _sets;
        VkDescriptorPool _pool = {};
        VkDescriptorSetLayout _layout = {};

        usize _inline_blocks = 0;
        usize _descriptor_buffer_size = 0;
        Buffer<BufferUsage::UniformBit> _inline_buffer;
};

class DescriptorSetAllocator : NonCopyable, public DeviceLinked  {

    using Key = core::Vector<VkDescriptorSetLayoutBinding>;

    struct LayoutPools : NonMovable {
        DescriptorSetLayout layout;
        core::Vector<std::unique_ptr<DescriptorSetPool>> pools;
    };

    public:
        DescriptorSetAllocator(DevicePtr dptr);

        DescriptorSetData create_descritptor_set(core::Span<Descriptor> descriptors);
        const DescriptorSetLayout& descriptor_set_layout(const Key& bindings);

        // Slow: for debug only
        usize layout_count() const;
        usize pool_count() const;
        usize free_sets() const;
        usize used_sets() const;

    private:
        LayoutPools& layout(const Key& bindings);

        std::unordered_map<Key, LayoutPools> _layouts;
        mutable std::mutex _lock;
};

}

#endif // YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORSETALLOCATOR_H

