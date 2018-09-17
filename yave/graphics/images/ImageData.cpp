/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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

namespace yave {

ImageData::ImageData(ImageData&& other) {
	swap(other);
}

ImageData& ImageData::operator=(ImageData&& other) {
	swap(other);
	return *this;
}

usize ImageData::byte_size(usize mip) const {
	auto s = size(mip);
	return (s.x() * s.y() * _format.bit_per_pixel()) / 8;
}

usize ImageData::layer_byte_size() const {
	usize data_size = 0;
	for(usize i = 0; i != _mips; ++i) {
		data_size += byte_size(i);
	}
	return data_size;
}

usize ImageData::combined_byte_size() const {
	return layer_byte_size() * _layers;
}

usize ImageData::layers() const {
	return _layers;
}

usize ImageData::mipmaps() const {
	return _mips;
}

const math::Vec3ui& ImageData::size() const {
	return _size;
}

math::Vec3ui ImageData::size(usize mip) const {
	return {std::max(u32(1), _size.x() >> mip), std::max(u32(1), _size.y() >> mip), std::max(u32(1), _size.z() >> mip)};
}

const ImageFormat& ImageData::format() const {
	return _format;
}

usize ImageData::data_offset(usize layer, usize mip) const {
	usize offset = layer ? layer_byte_size() * layer : 0;
	for(usize i = 0; i != mip; ++i) {
		offset += byte_size(i);
	}
	return offset;
}

const u8* ImageData::data(usize layer, usize mip) const {
	return _data.get() + data_offset(layer, mip);
}

void ImageData::swap(ImageData& other) {
	std::swap(_size, other._size);
	std::swap(_layers, other._layers);
	std::swap(_mips, other._mips);
	std::swap(_format, other._format);
	std::swap(_data, other._data);
}

ImageData::ImageData(const math::Vec2ui& size, const u8* data, ImageFormat format, u32 mips) :
		_size(size, 1),
		_format(format),
		_layers(1),
		_mips(mips) {

	usize data_size = combined_byte_size();
	_data = std::make_unique<u8[]>(data_size);
	std::memcpy(_data.get(), data, data_size);
}


}
