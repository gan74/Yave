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

		template<ImageUsage Usage>
		ImageView<Usage> get_image(FrameGraphImage res) const {
			if(!res.is_valid()) {
				return ImageView<Usage>();
			}
			return TransientImageView<Usage>(_images.find(res)->second);
		}

		void create_image(FrameGraphImage res, ImageFormat format, const math::Vec2ui& size, ImageUsage usage);
		void create_buffer(FrameGraphBuffer res, usize byte_size, BufferUsage usage);



		void release(FrameGraphImage res);
		void release(FrameGraphBuffer res);

		ImageBarrier barrier(FrameGraphImage res) const;
		BufferBarrier barrier(FrameGraphBuffer res) const;

		usize allocated_resources() const {
			return _images.size() + _buffers.size() + _released_images.size() + _released_buffers.size();
		}

	private:
		bool create_image_from_pool(TransientImage<>& res, ImageFormat format, const math::Vec2ui& size, ImageUsage usage);
		bool create_buffer_from_pool(TransientBuffer& res, usize byte_size, BufferUsage usage);


		std::unordered_map<FrameGraphImage, TransientImage<>> _images;
		std::unordered_map<FrameGraphBuffer, TransientBuffer> _buffers;


		core::Vector<TransientImage<>> _released_images;
		core::Vector<TransientBuffer> _released_buffers;
};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHRESOURCEPOOL_H
