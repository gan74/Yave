/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

FrameGraphDescriptorBinding::FrameGraphDescriptorBinding(const Descriptor& desc) : _type(BindingType::External), _external(desc) {
}

FrameGraphDescriptorBinding::FrameGraphDescriptorBinding(FrameGraphBufferId res, BindingType type) :
		_type(type), _buffer(res) {
}

FrameGraphDescriptorBinding::FrameGraphDescriptorBinding(FrameGraphImageId res, BindingType type) :
		_type(type), _image(res) {
}

FrameGraphDescriptorBinding FrameGraphDescriptorBinding::create_storage_binding(FrameGraphBufferId res) {
	return FrameGraphDescriptorBinding(res, BindingType::StorageBuffer);
}

FrameGraphDescriptorBinding FrameGraphDescriptorBinding::create_storage_binding(FrameGraphImageId res) {
	return FrameGraphDescriptorBinding(res, BindingType::StorageImage);
}

FrameGraphDescriptorBinding FrameGraphDescriptorBinding::create_uniform_binding(FrameGraphBufferId res) {
	return FrameGraphDescriptorBinding(res, BindingType::InputBuffer);
}

FrameGraphDescriptorBinding FrameGraphDescriptorBinding::create_uniform_binding(FrameGraphImageId res) {
	return FrameGraphDescriptorBinding(res, BindingType::InputImage);
}

/*Descriptor FrameGraphDescriptorBinding::create_and_save_descriptor(FrameGraphResourcePool* pool) {
	operator=(FrameGraphDescriptorBinding(create_descriptor(pool)));
	return _external;
}*/

Descriptor FrameGraphDescriptorBinding::create_descriptor(FrameGraphResourcePool* pool) const {
	switch(_type) {
		case BindingType::External:
			return _external;
		case BindingType::InputImage:
			return pool->image<ImageUsage::TextureBit>(_image);
		case BindingType::StorageImage:
			return pool->image<ImageUsage::StorageBit>(_image);
		case BindingType::InputBuffer:
			return pool->buffer<BufferUsage::UniformBit>(_buffer);
		case BindingType::StorageBuffer:
			return pool->buffer<BufferUsage::StorageBit>(_buffer);

		default:
		break;
	}
	return y_fatal("Invalid descriptor.");
}

}
