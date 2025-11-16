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
#ifndef YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORARRAY_H
#define YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORARRAY_H

#include <yave/graphics/descriptors/DescriptorSetProxy.h>

#include <y/core/HashMap.h>
#include <y/core/Vector.h>

namespace yave {

class DescriptorArray : NonMovable {
    public:
        static constexpr u32 upper_descriptor_count_limit = 1024 * 1024;
        static constexpr u32 reserved_descriptor_count = 16;

        ~DescriptorArray();

        VkDescriptorType descriptor_type() const;

        DescriptorSetProxy descriptor_set() const;

        VkDescriptorSetLayout descriptor_set_layout() const;

    protected:
        DescriptorArray(VkDescriptorType type, u32 starting_capacity = 1024);

        u32 add_descriptor(const Descriptor& desc);
        void remove_descriptor(u32 index);

        void add_descriptor_to_set(const Descriptor& desc, u32 index);

    private:
        VkWriteDescriptorSet descriptor_write(VkDescriptorSet set, const Descriptor& desc, u32 index) const;

        void alloc_set(u32 size);

        std::atomic<VkDescriptorSet> _set = {};
        u32 _capacity = 0;

        core::Vector<u32> _free;

        VkHandle<VkDescriptorSetLayout> _layout;
        VkHandle<VkDescriptorPool> _pool;

        const VkDescriptorType _type;

    protected:
        mutable ProfiledLock<std::recursive_mutex> _set_lock; // Locks for writes on the set
};



class ManagedDescriptorArray : public DescriptorArray {
    struct Entry {
        u32 index = 0;
        u32 ref_count = 0;
    };

    union DescriptorKey {
        VkDescriptorImageInfo image;
        VkDescriptorBufferInfo buffer;

        struct {
            u64 data[3];
        } raw;

        DescriptorKey() {
            std::fill(std::begin(raw.data), std::end(raw.data), u64(0));
        }

        bool operator==(const DescriptorKey& other) const {
            return std::equal(std::begin(raw.data), std::end(raw.data), std::begin(other.raw.data));
        }

        bool operator!=(const DescriptorKey& other) const {
            return !operator==(other);
        }
    };

    static_assert(sizeof(DescriptorKey) == 3 * sizeof(u64));

    struct Hasher {
        usize operator()(const DescriptorKey& key) const {
            return hash_range(std::begin(key.raw.data), std::end(key.raw.data));
        }
    };


    public:
        ~ManagedDescriptorArray();

    protected:
        ManagedDescriptorArray(VkDescriptorType type, u32 starting_capacity = 1024);

        u32 add_descriptor_managed(const Descriptor& desc);
        void remove_descriptor_managed(const Descriptor& desc);

    private:
        static DescriptorKey descriptor_key(const Descriptor& desc);

        core::FlatHashMap<DescriptorKey, Entry, Hasher> _descriptors;
};




}


#endif // YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORARRAY_H

