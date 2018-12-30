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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPHRESOURCEPOOL_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPHRESOURCEPOOL_H

#include <typeindex>

#include <yave/graphics/bindings/DescriptorSet.h>
#include <yave/graphics/framebuffer/Framebuffer.h>

#include "FrameGraphResource.h"
#include "FrameGraphPass.h"

#include <variant>

namespace yave {


class FrameGraphResourcePool : public DeviceLinked, NonCopyable {
	public:
		FrameGraphResourcePool(DevicePtr dptr);
		~FrameGraphResourcePool();

		template<ImageUsage Usage>
		ImageView<Usage> get_image(FrameGraphImage res) const {
			if(!res.is_valid()) {
				y_fatal("Invalid image resource.");
			}
			return TransientImageView<Usage>(_images.find(res)->second);
		}

		template<BufferUsage Usage>
		SubBuffer<Usage> get_buffer(FrameGraphBuffer res) const {
			if(!res.is_valid()) {
				y_fatal("Invalid buffer resource.");
			}
			return TransientSubBuffer<Usage>(_buffers.find(res)->second);
		}

		template<BufferUsage Usage, MemoryType Memory = prefered_memory_type(Usage), typename T>
		auto get_buffer(FrameGraphTypedBuffer<T> res) const {
			using BuffType = TransientSubBuffer<Usage, Memory>;
			using RetType = TypedSubBuffer<T, BuffType::usage, BuffType::memory_type>;
			if(!res.is_valid()) {
				y_fatal("Invalid buffer resource.");
			}
			return RetType(BuffType(_buffers.find(res)->second));
		}


		void create_image(FrameGraphImage res, ImageFormat format, const math::Vec2ui& size, ImageUsage usage);
		void create_buffer(FrameGraphBuffer res, usize byte_size, BufferUsage usage, MemoryType memory);

		void release(FrameGraphImage res);
		void release(FrameGraphBuffer res);

		ImageBarrier barrier(FrameGraphImage res) const;
		BufferBarrier barrier(FrameGraphBuffer res) const;

		usize allocated_resources() const;

		u32 create_resource_id();

	private:
		bool create_image_from_pool(TransientImage<>& res, ImageFormat format, const math::Vec2ui& size, ImageUsage usage);
		bool create_buffer_from_pool(TransientBuffer& res, usize byte_size, BufferUsage usage, MemoryType memory);


		std::unordered_map<FrameGraphImage, TransientImage<>> _images;
		std::unordered_map<FrameGraphBuffer, TransientBuffer> _buffers;

		core::Vector<TransientImage<>> _released_images;
		core::Vector<TransientBuffer> _released_buffers;

		u32 _next_id = 0;
};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHRESOURCEPOOL_H
