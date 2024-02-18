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
#ifndef YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORARRAY_H
#define YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORARRAY_H

#include <yave/graphics/descriptors/DescriptorSetBase.h>

#include <y/core/HashMap.h>
#include <y/core/Vector.h>
#include <y/concurrent/Mutexed.h>

namespace yave {

class DescriptorArray : NonMovable {
    struct Entry {
        u32 index = 0;
        u32 ref_count = 0;
    };

    struct DescriptorArraySet : DescriptorSetBase {
        DescriptorArraySet(VkDescriptorSet set) {
            _set = set;
        }
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
        static constexpr u32 upper_descriptor_count_limit = 1024 * 1024;

        ~DescriptorArray();

        usize descriptor_count() const;

        DescriptorSetBase descriptor_set() const;

        VkDescriptorSetLayout descriptor_set_layout() const;

    protected:
        DescriptorArray(VkDescriptorType type, u32 starting_capacity = 1024);

        u32 add_descriptor(const Descriptor& desc);
        void remove_descriptor(const Descriptor& desc);

    private:
        static DescriptorKey descriptor_key(const Descriptor& desc);

        VkWriteDescriptorSet descriptor_write(VkDescriptorSet set, const DescriptorKey& desc, u32 index) const;
        void add_descriptor_to_set(const DescriptorKey& desc, u32 index);

        struct Allocator {
            core::FlatHashMap<DescriptorKey, Entry, Hasher> descriptors;
            core::Vector<u32> free;
            VkHandle<VkDescriptorPool> pool;
            u32 capacity = 0;

            VkDescriptorSet alloc_set(u32 size, const DescriptorArray* parent);
        };

        VkHandle<VkDescriptorSetLayout> _layout;
        std::atomic<VkDescriptorSet> _set = {};

        concurrent::Mutexed<Allocator> allocator;

        mutable std::mutex _set_lock; // Locks for writes on the set

        const VkDescriptorType _type;
};

}


#endif // YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORARRAY_H

