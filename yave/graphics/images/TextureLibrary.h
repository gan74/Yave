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
#ifndef YAVE_GRAPHICS_IMAGES_TEXTURELIBRARY_H
#define YAVE_GRAPHICS_IMAGES_TEXTURELIBRARY_H

#include <yave/graphics/descriptors/DescriptorSetBase.h>

#include "ImageView.h"

#include <y/core/HashMap.h>

#include <mutex>

namespace yave {

class TextureLibrary final : NonMovable {
    struct Entry {
        u32 index = 0;
        u32 ref_count = 0;
    };

    class LibrarySet : public DescriptorSetBase {
        public:
            LibrarySet(VkDescriptorPool pool, VkDescriptorSetLayout layout, u32 desc_count);
    };

    public:
        TextureLibrary();
        ~TextureLibrary();

        u32 add_texture(const TextureView& tex);
        void remove_texture(const TextureView& tex);

        usize texture_count() const;

        DescriptorSetBase descriptor_set() const;

        VkDescriptorSetLayout descriptor_set_layout() const;

    private:
        void add_texture_to_set(const TextureView& tex, u32 index);

        core::FlatHashMap<VkImageView, Entry> _textures;
        core::Vector<u32> _free;

        u32 _max_desc_count = 1024;

        VkHandle<VkDescriptorPool> _pool;
        VkHandle<VkDescriptorSetLayout> _layout;
        LibrarySet _set;

        mutable std::mutex _map_lock;
        mutable std::mutex _set_lock;
};

}


#endif // YAVE_GRAPHICS_IMAGES_TEXTURELIBRARY_H

