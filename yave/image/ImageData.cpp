/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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

#include <y/io/BuffReader.h>
#include <y/io/Decoder.h>

namespace yave {

ImageData::ImageData(ImageData&& other) : ImageData() {
	swap(other);
}

ImageData& ImageData::operator=(ImageData&& other) {
	swap(other);
	return *this;
}

usize ImageData::byte_size() const {
	return (_size.x() *_size.y() * _format.bit_per_pixel()) / 8;
}

usize ImageData::all_mip_bytes_size() const {
	usize data_size = 0;
	for(usize i = 0; i != _mips; ++i) {
		auto size = mip_size(i);
		data_size += (size.x() * size.y() * _format.bit_per_pixel()) / 8;
	}
	return data_size;
}

usize ImageData::mipmaps() const {
	return _mips;
}

math::Vec2ui ImageData::mip_size(usize lvl) const {
	usize factor = 1 << lvl;
	return {std::max(usize(1), _size.x() / factor), std::max(usize(1), _size.y() / factor)};
}

const math::Vec2ui& ImageData::size() const {
	return _size;
}

const ImageFormat& ImageData::format() const {
	return _format;
}

const u8* ImageData::data() const {
	return _data.as_ptr();
}

void ImageData::swap(ImageData& other) {
	std::swap(_size, other._size);
	std::swap(_mips, other._mips);
	std::swap(_format, other._format);
	std::swap(_data, other._data);
}

ImageData ImageData::from_file(io::ReaderRef reader) {
	const char* err_msg = "Unable to read image.";
	struct Header {
		u32 magic;
		u32 type;
		u32 version;

		u32 width;
		u32 height;
		u32 mips;
		u32 format;

		bool is_valid() const {
			return magic == 0x65766179 &&
				   type == 2 &&
				   version == 2 &&
				   format > 0;
		}
	};

	auto decoder = io::Decoder(reader);

	Header header = decoder.decode<Header>().expected(err_msg);
	if(!header.is_valid()) {
		fatal(err_msg);
	}

	ImageData data;
	data._size = {header.width, header.height};
	data._mips = header.mips;
	data._format = ImageFormat(vk::Format(header.format));

	usize data_size = data.all_mip_bytes_size();

	data._data = new u8[data_size];

	reader->read(data._data, data_size).expected(err_msg);

	return data;
}


}
