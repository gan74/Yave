/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_BUFFERS_MULTIBUFFERWRAPPER_H
#define YAVE_GRAPHICS_BUFFERS_MULTIBUFFERWRAPPER_H

#include <yave/graphics/swapchain/FrameToken.h>

namespace yave {

template<typename Buffer>
class MultiBufferWrapper {

	using subbuffer_t = typename Buffer::sub_buffer_type;
	using buffer_t = typename Buffer::base_buffer_type;

	public:
		MultiBufferWrapper(usize size) : _size(size) {
		}

		auto operator[](const FrameToken& token) {
			lazy_init(token);
			return subbuffer_t(_buffer, _size, _aligned_byte_size * token.image_index);
		}

	private:
		static usize align_size(usize total_byte_size, usize alignent) {
			return (total_byte_size + alignent - 1) & ~(alignent - 1);
		}

		void lazy_init(const FrameToken& token) {
			if(is_initialized()) {
				return;
			}
			DevicePtr dptr = token.image_view.device();

			usize byte_size = Buffer::total_byte_size(_size);
			_aligned_byte_size = align_size(byte_size, subbuffer_t::alignment(dptr));

			_buffer = buffer_t(dptr, _aligned_byte_size * token.image_count);
		}

		bool is_initialized() const {
			return _buffer.device();
		}

		buffer_t _buffer;
		usize _size = 0;
		usize _aligned_byte_size = 0;
};


}

#endif // YAVE_GRAPHICS_BUFFERS_MULTIBUFFERWRAPPER_H
