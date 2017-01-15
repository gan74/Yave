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

const u8* ImageData::raw_pixel(const math::Vec2ui& pos) {
	return _data +
			((_size.x() *_bpp) *pos.x()) +
			(_bpp *pos.y())
		;
}

const u8* ImageData::raw_data() const {
	return _data;
}

void ImageData::swap(ImageData& other) {
	std::swap(_size, other._size);
	std::swap(_bpp, other._bpp);
	std::swap(_data, other._data);
}

ImageData ImageData::from_file(io::ReaderRef reader) {
	const char* err_msg = "Unable to read image.";

	auto decoder = io::Decoder(reader);

	u32 magic = decoder.decode<u32>().expected(err_msg);
	u64 version = decoder.decode<u64>().expected(err_msg);

	ImageData data;
	if(magic != 0x65766179 || version != ((u64(1) << 63) | 1)) {
		log_msg(err_msg, LogType::Error);
		return data;
	}

	u32 height = decoder.decode<u32>().expected(err_msg);
	u32 width = decoder.decode<u32>().expected(err_msg);
	u32 mips = decoder.decode<u32>().expected(err_msg);
	unused(mips);

	u32 bpp = 4;

	u32 data_size = width * height * bpp;

	data._bpp = bpp;
	data._size = math::vec(width, height);
	data._data = new u8[data_size];

	reader->read(data._data, data_size).expected(err_msg);

	return data;
}

}
