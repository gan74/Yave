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

#include "FrameGraphDescriptorBinding.h"
#include "FrameGraphResourcePool.h"

namespace yave {

FrameGraphDescriptorBinding::FrameGraphDescriptorBinding(const Binding& bind) : _type(BindingType::External), _external(bind) {
}

FrameGraphDescriptorBinding::FrameGraphDescriptorBinding(FrameGraphImageId img) : _type(BindingType::ImageResource), _image(img) {
}

FrameGraphDescriptorBinding::FrameGraphDescriptorBinding(FrameGraphBufferId buf) : _type(BindingType::BufferResource), _buffer(buf) {
}

Binding FrameGraphDescriptorBinding::create_binding(FrameGraphResourcePool* pool) const {
	switch(_type) {
		case BindingType::External:
			return _external;
		case BindingType::ImageResource:
			return pool->get_image<ImageUsage::TextureBit>(_image);
		case BindingType::BufferResource:
			return pool->get_buffer<BufferUsage::UniformBit>(_buffer);

		default:
		break;
	}
	return y_fatal("Invalid descriptor.");
}

}
