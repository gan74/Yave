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
#ifndef YAVE_GRAPHICS_BARRIERS_BARRIER_H
#define YAVE_GRAPHICS_BARRIERS_BARRIER_H

#include "PipelineStage.h"

#include <yave/graphics/images/ImageView.h>
#include <yave/graphics/buffers/Buffer.h>
#include <yave/graphics/buffers/SubBuffer.h>

#include <y/core/Result.h>

namespace yave {

vk::ImageMemoryBarrier create_image_barrier(vk::Image image, ImageFormat format, usize layers, usize mips, vk::ImageLayout old_layout, vk::ImageLayout new_layout);

class ImageBarrier {
	public:
		ImageBarrier(const ImageBase& image, PipelineStage src, PipelineStage dst);

		vk::ImageMemoryBarrier vk_barrier() const {
			return _barrier;
		}

		PipelineStage dst_stage() const {
			return _dst;
		}

		PipelineStage src_stage() const {
			return _src;
		}

	private:
		vk::ImageMemoryBarrier _barrier;
		PipelineStage _src;
		PipelineStage _dst;
};

class BufferBarrier {
	public:
		BufferBarrier(const BufferBase& buffer, PipelineStage src, PipelineStage dst);
		BufferBarrier(const SubBufferBase& buffer, PipelineStage src, PipelineStage dst);

		vk::BufferMemoryBarrier vk_barrier() const {
			return _barrier;
		}

		PipelineStage dst_stage() const {
			return _dst;
		}

		PipelineStage src_stage() const {
			return _src;
		}

	private:
		vk::BufferMemoryBarrier _barrier;
		PipelineStage _src;
		PipelineStage _dst;
};


/*class GenericBarrier {

	template<typename T>
	auto create_barrier(const T& t) {
		if constexpr(std::is_constructible<ImageBarrier, T>::value) {
			return ImageBarrier(t);
		} else {
			return BufferBarrier(t);
		}
	}

	public:
		enum class Type {
			Image,
			Buffer
		};

		GenericBarrier(const ImageBarrier& barrier);
		GenericBarrier(const BufferBarrier& barrier);

		template<typename T>
		GenericBarrier(const T& t) : GenericBarrier(create_barrier(t)) {
		}

		core::Result<ImageBarrier, BufferBarrier> image_barrier() const;
		core::Result<BufferBarrier, ImageBarrier> buffer_barrier() const;

	private:
		Type _type;
		union {
			ImageBarrier _image;
			BufferBarrier _buffer;
		};
};*/

}


#endif // YAVE_GRAPHICS_BARRIERS_BARRIER_H
