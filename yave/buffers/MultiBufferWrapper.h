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
#ifndef YAVE_BUFFERS_MULTIBUFFERWRAPPER_H
#define YAVE_BUFFERS_MULTIBUFFERWRAPPER_H

#include <yave/swapchain/FrameToken.h>

namespace yave {

template<typename Buffer>
class MultiBufferWrapper {

	using subbuffer_t = SubBuffer<Buffer::usage, Buffer::memory_type, Buffer::buffer_transfer>;

	public:
		MultiBufferWrapper(usize size) : _size(size) {
		}

		auto operator[](const FrameToken& token) {
			lazy_init(token);
			return typename Buffer::sub_buffer_type(_buffer, _size * token.image_index, _size);
		}

	private:
		void lazy_init(const FrameToken& token) {
			if(is_initialized()) {
				return;
			}
			DevicePtr dptr = token.image_view.device();
			_buffer = Buffer(dptr, _size * token.image_count);
		}

		bool is_initialized() const {
			return _buffer.device();
		}

		Buffer _buffer;
		usize _size = 0;
};


}

#endif // YAVE_BUFFERS_MULTIBUFFERWRAPPER_H
