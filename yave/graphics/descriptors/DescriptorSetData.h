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
#ifndef YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORSETDATA_H
#define YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORSETDATA_H

#include <yave/yave.h>

#include <yave/graphics/vk/vk.h>

namespace yave {
class DescriptorSetData {
    public:
        DescriptorSetData() = default;

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
}

#endif // YAVE_GRAPHICS_DESCRIPTORS_DESCRIPTORSETDATA_H

