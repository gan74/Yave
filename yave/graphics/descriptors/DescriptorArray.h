/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include <mutex>

namespace yave {

class DescriptorArray : NonMovable {
    struct Entry {
        u32 index = 0;
        u32 ref_count = 0;
    };

    class ArraySet : public DescriptorSetBase {
        public:
            ArraySet(VkDescriptorPool pool, VkDescriptorSetLayout layout, u32 desc_count);
    };

    union DescriptorKey {
        struct {
            VkImageView view;
            VkSampler sampler;
        } image;

        struct {
            VkBuffer buffer;
            u64 offset;
        } buffer;

        struct {
            u64 data[2];
        } raw;

        bool operator==(const DescriptorKey& other) const {
            return raw.data[0] == other.raw.data[0] && raw.data[1] == other.raw.data[1];
        }

        bool operator!=(const DescriptorKey& other) const {
            return !operator==(other);
        }
    };

    static_assert(sizeof(DescriptorKey) == 2 * sizeof(u64));

    struct Hasher {
        usize operator()(const DescriptorKey& key) const {
            return hash_range(std::begin(key.raw.data), std::end(key.raw.data));
        }
    };

    public:
        ~DescriptorArray();

        usize descriptor_count() const;

        DescriptorSetBase descriptor_set() const;

        VkDescriptorSetLayout descriptor_set_layout() const;

    protected:
        DescriptorArray(VkDescriptorType type, u32 max_desc = 1024);

        u32 add_descriptor(const Descriptor& desc);
        void remove_descriptor(const Descriptor& desc);

    private:
        static DescriptorKey descriptor_key(const Descriptor& desc);

        void add_descriptor_to_set(const Descriptor& desc, u32 index);

        core::FlatHashMap<DescriptorKey, Entry, Hasher> _descriptors;
        core::Vector<u32> _free;

        VkHandle<VkDescriptorPool> _pool;
        VkHandle<VkDescriptorSetLayout> _layout;
        ArraySet _set;

        mutable std::mutex _map_lock;
        mutable std::mutex _set_lock;

        const u32 _max_descriptors = 1024;
        const VkDescriptorType _type;
};

}


#endif // YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORARRAY_H

