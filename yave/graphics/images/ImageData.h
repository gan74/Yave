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
#ifndef YAVE_GRAPHICS_IMAGES_IMAGEDATA_H
#define YAVE_GRAPHICS_IMAGES_IMAGEDATA_H

#include <y/reflect/reflect.h>
#include <y/math/Vec.h>
#include <y/core/FixedArray.h>

#include "ImageFormat.h"

namespace yave {

class ImageData : NonCopyable {

    public:
        struct Mip {
            core::Span<byte> data;
            math::Vec3ui size;
        };

        ImageData() = default;
        ImageData(const math::Vec2ui& size, const void* data, ImageFormat format, usize mips = 1);

        static usize mip_count(const math::Vec3ui& size);
        static math::Vec3ui mip_size(const math::Vec3ui& size, usize mip = 0);
        static usize mip_byte_size(const math::Vec3ui& size, ImageFormat format, usize mip = 0);
        static usize byte_size(const math::Vec3ui& size, ImageFormat format, usize mips);
        static math::Vec3ui with_block_size(math::Vec3ui size, ImageFormat format);

        usize mip_byte_size(usize mip) const;
        usize byte_size() const;

        const math::Vec3ui& size() const;
        math::Vec3ui mip_size(usize mip) const;

        const ImageFormat& format() const;

        usize mipmaps() const;

        usize data_offset(usize mip = 0) const;
        Mip mip_data(usize mip = 0) const;

        const byte* data() const;

        y_reflect(ImageData, _size, _format, _mips, _data)

    private:
        math::Vec3ui _size = math::Vec3ui(0, 0, 1);
        ImageFormat _format;

        u32 _mips = 1;

        core::FixedArray<byte> _data;
};

}

#endif // YAVE_GRAPHICS_IMAGES_IMAGEDATA_H

