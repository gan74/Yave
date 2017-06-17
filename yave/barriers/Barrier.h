/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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
#ifndef YAVE_BARRIERS_BARRIER_H
#define YAVE_BARRIERS_BARRIER_H

#include "PipelineStage.h"

#include <yave/image/ImageBase.h>
#include <yave/buffer/Buffer.h>
#include <yave/buffer/SubBuffer.h>

namespace yave {

vk::ImageMemoryBarrier create_image_barrier(vk::Image image, ImageFormat format, usize mips, vk::ImageLayout old_layout, vk::ImageLayout new_layout);

class ImageBarrier {
	public:
		ImageBarrier(const ImageBase& image) :
				_image(image.vk_image()),
				_usage(image.usage()),
				_format(image.format()),
				_mips(image.mipmaps()) {
		}

		vk::ImageMemoryBarrier vk_barrier() const;

	private:
		vk::Image _image;
		ImageUsage _usage;
		ImageFormat _format;
		usize _mips;
};

class BufferBarrier {
	public:
		template<BufferUsage Usage>
		BufferBarrier(const Buffer<Usage, MemoryFlags::DeviceLocal>& buffer) :
				_buffer(buffer.vk_buffer()),
				_size(buffer.byte_size()),
				_offset(0) {
		}

		template<BufferUsage Usage>
		BufferBarrier(const SpecializedSubBuffer<Usage, MemoryFlags::DeviceLocal> buffer) :
				_buffer(buffer.vk_buffer()),
				_size(buffer.byte_size()),
				_offset(buffer.byte_offset()) {
		}

		vk::BufferMemoryBarrier vk_barrier() const;

	private:
		vk::Buffer _buffer;
		usize _size;
		usize _offset;
};

}


#endif // YAVE_BARRIERS_BARRIER_H
