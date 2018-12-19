/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include "FrameGraphResourcePool.h"

namespace yave {

static void check_res(FrameGraphResourceBase res) {
	if(!res.is_valid()) {
		y_fatal("Invalid resource.");
	}
}

FrameGraphResourcePool::FrameGraphResourcePool(DevicePtr dptr) : DeviceLinked(dptr) {
}

void FrameGraphResourcePool::create_image(FrameGraphImage res, ImageFormat format, const math::Vec2ui& size, ImageUsage usage) {
	check_res(res);
	auto& image = _images[res];
	if(image.device()) {
		y_fatal("Image already exists.");
	}
	image = TransientImage<>(device(), format, usage, size);
}

void FrameGraphResourcePool::create_buffer(FrameGraphBuffer res, usize byte_size, BufferUsage usage) {
	check_res(res);
	auto& buffer = _buffers[res];
	if(buffer.device()) {
		y_fatal("Buffer already exists.");
	}
	buffer = TransientBuffer(device(), byte_size, usage);
}

ImageBarrier FrameGraphResourcePool::barrier(FrameGraphImage res) const {
	check_res(res);
	return _images.find(res)->second;
}

BufferBarrier FrameGraphResourcePool::barrier(FrameGraphBuffer res) const {
	check_res(res);
	return _buffers.find(res)->second;
}

}
