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

#include <yave/images/ImageView.h>
#include <yave/buffers/Buffer.h>
#include <yave/buffers/SubBuffer.h>

namespace yave {

vk::ImageMemoryBarrier create_image_barrier(vk::Image image, ImageFormat format, usize layers, usize mips, vk::ImageLayout old_layout, vk::ImageLayout new_layout);

class ImageBarrier {
	public:
		ImageBarrier(const ImageBase& image) :
				_image(image.vk_image()),
				_usage(image.usage()),
				_format(image.format()),
				_layers(image.layers()),
				_mips(image.mipmaps()) {
		}

		template<ImageUsage Usage>
		ImageBarrier(const ImageView<Usage>& image) : ImageBarrier(image.image()) {
		}

		vk::ImageMemoryBarrier vk_barrier() const;

	private:
		vk::Image _image;
		ImageUsage _usage;
		ImageFormat _format;
		usize _layers;
		usize _mips;
};

class BufferBarrier {
	public:
		template<BufferUsage Usage, BufferTransfer Transfer>
		BufferBarrier(const Buffer<Usage, MemoryType::DeviceLocal, Transfer>& buffer) :
				_buffer(buffer.vk_buffer()),
				_size(buffer.byte_size()),
				_offset(0) {
		}

		BufferBarrier(const SubBuffer<BufferUsage::None, MemoryType::DeviceLocal> buffer) :
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
