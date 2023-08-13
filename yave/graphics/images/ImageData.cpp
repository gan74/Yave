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

#include "ImageData.h"

#include <cstring>

namespace yave {

usize ImageData::mip_count(const math::Vec3ui& size) {
    usize l = 0;
    while(mip_size(size, l).max_component() != 1) {
        ++l;
    }
    return l + 1;
}

math::Vec3ui ImageData::mip_size(const math::Vec3ui& size, usize mip) {
    return {std::max(u32(1), size.x() >> mip), std::max(u32(1), size.y() >> mip), std::max(u32(1), size.z() >> mip)};
}

usize ImageData::mip_byte_size(const math::Vec3ui& size, ImageFormat format, usize mip) {
    const math::Vec3ui s = with_block_size(mip_size(size, mip), format);
    const usize bits = (s.x() * s.y() * s.z() * format.bit_per_pixel());
    y_debug_assert(bits % 8 == 0);
    return bits / 8;
}

usize ImageData::byte_size(const math::Vec3ui& size, ImageFormat format, usize mips) {
    usize data_size = 0;
    for(usize i = 0; i != mips; ++i) {
        data_size += mip_byte_size(size, format, i);
    }
    return data_size;
}

math::Vec3ui ImageData::with_block_size(math::Vec3ui size, ImageFormat format) {
    if(format.is_block_format()) {
        const math::Vec3ui block_size = format.block_size();
        for(usize i = 0; i != 3; ++i) {
            const u32 m = size[i] % block_size[i];
            size[i] += m ? block_size[i] - m : 0u;
            y_debug_assert(size[i] % block_size[i] == 0);
        }
    }
    return size;
}

usize ImageData::mip_byte_size(usize mip) const {
    return mip_byte_size(_size, _format, mip);
}

usize ImageData::byte_size() const {
    return byte_size(_size, _format, _mips);
}

usize ImageData::mipmaps() const {
    return _mips;
}

const math::Vec3ui& ImageData::size() const {
    return _size;
}

math::Vec3ui ImageData::mip_size(usize mip) const {
    return mip_size(_size, mip);
}

const ImageFormat& ImageData::format() const {
    return _format;
}

usize ImageData::data_offset(usize mip) const {
    usize offset = 0;
    for(usize i = 0; i != mip; ++i) {
        offset += mip_byte_size(i);
    }
    return offset;
}

const u8* ImageData::data() const {
    return _data.data();
}

ImageData::Mip ImageData::mip_data(usize mip) const {
    return Mip {
        core::Span<u8>(_data.data() + data_offset(mip), mip_byte_size(mip)),
        mip_size(mip)
    };
}

ImageData::ImageData(const math::Vec2ui& size, const void* data, ImageFormat format, usize mips) :
        _size(size, 1),
        _format(format),
        _mips(u32(mips)) {

    const usize data_size = byte_size();
    _data = core::FixedArray<u8>(data_size);
    std::memcpy(_data.data(), data, data_size);
}

}

