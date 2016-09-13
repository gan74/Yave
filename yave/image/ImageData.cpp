/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#include "ImageData.h"

#include <y/io/BuffReader.h>
#include <y/io/Decoder.h>

namespace yave {

ImageData::ImageData() : _size(0), _bpp(0), _data(nullptr) {
}

ImageData::~ImageData() {
	delete[] _data;
}

ImageData::ImageData(ImageData&& other) : ImageData() {
	swap(other);
}

ImageData& ImageData::operator=(ImageData&& other) {
	swap(other);
	return *this;
}

usize ImageData::byte_size() const {
	return _size.x() *_size.y() *_bpp;
}

const math::Vec2ui& ImageData::size() const {
	return _size;
}

const u8* ImageData::get_raw_pixel(const math::Vec2ui& pos) {
	return _data +
			((_size.x() *_bpp) *pos.x()) +
			(_bpp *pos.y())
		;
}

const u8* ImageData::get_raw_data() const {
	return _data;
}

void ImageData::swap(ImageData& other) {
	std::swap(_size, other._size);
	std::swap(_bpp, other._bpp);
	std::swap(_data, other._data);
}

ImageData ImageData::from_file(io::ReaderRef reader) {
	auto decoder = io::Decoder(reader);

	u32 height = 0;
	u32 width = 0;
	u64 data_size = 0;

	decoder.decode<io::Byteorder::BigEndian>(width);
	decoder.decode<io::Byteorder::BigEndian>(height);
	decoder.decode<io::Byteorder::BigEndian>(data_size);

	if(height *width *4 != data_size) {
		fatal("Invalid file size");
	}

	ImageData data;
	data._bpp = 4;
	data._size = math::vec(width, height);
	data._data = new u8[data_size];

	if(reader->read(data._data, data_size) != data_size) {
		fatal("Invalid file size");
	}

	return data;
}

}
