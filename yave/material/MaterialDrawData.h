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
#ifndef YAVE_MATERIAL_MATERIALDRAWDATA_H
#define YAVE_MATERIAL_MATERIALDRAWDATA_H

#include "MaterialData.h"

#include <yave/graphics/images/ImageView.h>

namespace yave {

class MaterialDrawData : NonCopyable {
    public:
        MaterialDrawData() = default;

        MaterialDrawData(MaterialDrawData&& other);
        MaterialDrawData& operator=(MaterialDrawData&& other);

        ~MaterialDrawData();

        bool is_null() const;
        u32 index() const;

    private:
        friend class LifetimeManager;
        friend class MaterialAllocator;

        void recycle();

    private:
        void swap(MaterialDrawData& other);

        u32 _index = u32(-1);

        std::array<TextureView, MaterialData::max_texture_count> _textures;
        MaterialAllocator* _parent = nullptr;
};

}

#endif // YAVE_MATERIAL_MATERIALDRAWDATA_H

