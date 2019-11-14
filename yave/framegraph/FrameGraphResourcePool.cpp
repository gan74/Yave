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

#include "FrameGraphResourcePool.h"

namespace yave {

template<typename U>
static void check_usage(U u) {
	if(u == U::None) {
		y_fatal("Invalid resource usage.");
	}
}

FrameGraphResourcePool::FrameGraphResourcePool(DevicePtr dptr) : DeviceLinked(dptr) {
}

FrameGraphResourcePool::~FrameGraphResourcePool() {
	if(!_images.empty() || !_buffers.empty()) {
		y_fatal("Not all resources have been released.");
	}
}

void FrameGraphResourcePool::create_image(FrameGraphImageId res, ImageFormat format, const math::Vec2ui& size, ImageUsage usage) {
	res.check_valid();
	check_usage(usage);

	auto& image = _images[res];
	if(image) {
		y_fatal("Image already exists.");
	}

	if(!(image = create_image_from_pool(format, size, usage))) {
		_image_storage << std::make_unique<ImageContainer>(device(), format, usage, size);
		image = _image_storage.last().get();
	}
}

void FrameGraphResourcePool::create_buffer(FrameGraphBufferId res, usize byte_size, BufferUsage usage, MemoryType memory) {
	res.check_valid();
	check_usage(usage);

	if(memory == MemoryType::DontCare) {
		memory = prefered_memory_type(usage);
	}
	auto& buffer = _buffers[res];
	if(!buffer.is_null()) {
		y_fatal("Buffer already exists.");
	}
	if(!create_buffer_from_pool(buffer, byte_size, usage, memory)) {
		buffer = TransientBuffer(device(), byte_size, usage, memory);
	}
}


void FrameGraphResourcePool::create_alias(FrameGraphImageId dst, FrameGraphImageId src) {
	dst.check_valid();
	src.check_valid();

	const auto& orig = _images[src];
	if(!orig) {
		y_fatal("Source image doesn't exists.");
	}

	auto& image = _images[dst];
	if(image) {
		y_fatal("Destination image already exists.");
	}

	image = orig;
}


FrameGraphResourcePool::ImageContainer* FrameGraphResourcePool::create_image_from_pool(ImageFormat format, const math::Vec2ui& size, ImageUsage usage) {
	for(auto it = _released_images.begin(); it != _released_images.end(); ++it) {
		ImageContainer* img = *it;
		if(img->image.format() == format && img->image.size() == size && img->image.usage() == usage) {
			y_debug_assert(img->aliases == 0);
			_released_images.erase_unordered(it);
			return img;
		}
	}
	return nullptr;
}


bool FrameGraphResourcePool::create_buffer_from_pool(TransientBuffer& res, usize byte_size, BufferUsage usage, MemoryType memory) {
	for(auto it = _released_buffers.begin(); it != _released_buffers.end(); ++it) {
		auto& buffer = *it;

		if(buffer.byte_size() == byte_size &&
		   buffer.usage() == usage &&
		   (buffer.memory_type() == memory || memory == MemoryType::DontCare)) {

			res = std::move(buffer);
			_released_buffers.erase_unordered(it);
			return true;
		}
	}
	return false;
}


void FrameGraphResourcePool::release(FrameGraphImageId res) {
	res.check_valid();
	if(const auto it = _images.find(res); it != _images.end()) {
		if(it->second->aliases) {
			--(it->second->aliases);
		} else {
			_released_images << it->second;
			_images.erase(it);
		}
	} else {
		y_fatal("Released image resource does not belong to pool.");
	}
}

void FrameGraphResourcePool::release(FrameGraphBufferId res) {
	res.check_valid();
	if(const auto it = _buffers.find(res); it != _buffers.end()) {
		_released_buffers << std::move(it->second);
		_buffers.erase(it);
	} else {
		y_fatal("Released buffer resource does not belong to pool.");
	}
}

bool FrameGraphResourcePool::is_alive(FrameGraphImageId res) const {
	return _images.find(res) != _images.end();
}

bool FrameGraphResourcePool::is_alive(FrameGraphBufferId res) const {
	return _buffers.find(res) != _buffers.end();
}

bool FrameGraphResourcePool::are_aliased(FrameGraphImageId a, FrameGraphImageId b) const {
	a.check_valid();
	b.check_valid();
	if(const auto a_it = _images.find(a); a_it != _images.end()) {
		if(const auto b_it = _images.find(b); b_it != _images.end()) {
			return a_it->second == b_it->second;
		}
	}
	return false;
}

ImageBarrier FrameGraphResourcePool::barrier(FrameGraphImageId res, PipelineStage src, PipelineStage dst) const {
	res.check_valid();
	return ImageBarrier(_images.find(res)->second->image, src, dst);
}

BufferBarrier FrameGraphResourcePool::barrier(FrameGraphBufferId res, PipelineStage src, PipelineStage dst) const {
	res.check_valid();
	return BufferBarrier(_buffers.find(res)->second, src, dst);
}

const ImageBase& FrameGraphResourcePool::image_base(FrameGraphImageId res) const {
	return find(res);
}

const BufferBase& FrameGraphResourcePool::buffer_base(FrameGraphBufferId res) const {
	return find(res);
}

usize FrameGraphResourcePool::allocated_resources() const {
	return _buffers.size() + _released_buffers.size() + _image_storage.size();
}

u32 FrameGraphResourcePool::create_resource_id() {
	return _next_id++;
}


const TransientImage<>& FrameGraphResourcePool::find(FrameGraphImageId res) const {
	if(!res.is_valid()) {
		y_fatal("Invalid image resource.");
	}
	if(const auto it = _images.find(res); it != _images.end()) {
		return it->second->image;
	}

	return y_fatal("Image resource doesn't exist.");
}

const TransientBuffer& FrameGraphResourcePool::find(FrameGraphBufferId res) const {
	if(!res.is_valid()) {
		y_fatal("Invalid buffer resource.");
	}
	if(const auto it = _buffers.find(res); it != _buffers.end()) {
		return it->second;
	}
	return y_fatal("Buffer resource doesn't exist.");
}

}
