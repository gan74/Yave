/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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

#include "FrameGraphFrameResources.h"
#include "FrameGraphResourcePool.h"

namespace yave {

FrameGraphFrameResources::FrameGraphFrameResources(std::shared_ptr<FrameGraphResourcePool> pool) : _pool(pool) {
}

FrameGraphFrameResources::~FrameGraphFrameResources() {
    for(auto&& res : _image_storage) {
        _pool->release(std::move(res));
    }
    for(auto&& res : _buffer_storage) {
        _pool->release(std::move(res));
    }
    _pool->garbage_collect();
}

DevicePtr FrameGraphFrameResources::device() const {
    return _pool->device();
}

u32 FrameGraphFrameResources::create_resource_id() {
    return _next_id++;
}


void FrameGraphFrameResources::reserve(usize images, usize buffers) {
    _images.reserve(images);
    _buffers.reserve(buffers);
}


void FrameGraphFrameResources::create_image(FrameGraphImageId res, ImageFormat format, const math::Vec2ui& size, ImageUsage usage) {
    res.check_valid();

    auto& image = _images[res];
    if(image) {
        y_fatal("Buffer already exists.");
    }
    _image_storage.emplace_back(_pool->create_image(format, size, usage));
    image = &_image_storage.back();
}

void FrameGraphFrameResources::create_buffer(FrameGraphBufferId res, usize byte_size, BufferUsage usage, MemoryType memory) {
    res.check_valid();

    auto& buffer = _buffers[res];
    if(buffer) {
        y_fatal("Buffer already exists.");
    }
    _buffer_storage.emplace_back(_pool->create_buffer(byte_size, usage, memory));
    buffer = &_buffer_storage.back();
}

bool FrameGraphFrameResources::is_alive(FrameGraphImageId res) const {
    return _images.find(res) != _images.end();
}

bool FrameGraphFrameResources::is_alive(FrameGraphBufferId res) const {
    return _buffers.find(res) != _buffers.end();
}

ImageBarrier FrameGraphFrameResources::barrier(FrameGraphImageId res, PipelineStage src, PipelineStage dst) const {
    res.check_valid();
    return ImageBarrier(*_images.find(res)->second, src, dst);
}

BufferBarrier FrameGraphFrameResources::barrier(FrameGraphBufferId res, PipelineStage src, PipelineStage dst) const {
    res.check_valid();
    return BufferBarrier(*_buffers.find(res)->second, src, dst);
}

const ImageBase& FrameGraphFrameResources::image_base(FrameGraphImageId res) const {
    return find(res);
}

const BufferBase& FrameGraphFrameResources::buffer_base(FrameGraphBufferId res) const {
    return find(res);
}


void FrameGraphFrameResources::create_alias(FrameGraphImageId dst, FrameGraphImageId src) {
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

bool FrameGraphFrameResources::are_aliased(FrameGraphImageId a, FrameGraphImageId b) const {
    return &find(a) == &find(b);
}







const TransientImage<>& FrameGraphFrameResources::find(FrameGraphImageId res) const {
    if(!res.is_valid()) {
        y_fatal("Invalid image resource.");
    }
    if(const auto it = _images.find(res); it != _images.end()) {
        return *it->second;
    }

    /*return*/ y_fatal("Image resource doesn't exist.");
}

const TransientBuffer& FrameGraphFrameResources::find(FrameGraphBufferId res) const {
    if(!res.is_valid()) {
        y_fatal("Invalid buffer resource.");
    }
    if(const auto it = _buffers.find(res); it != _buffers.end()) {
        return *it->second;
    }
    /*return*/ y_fatal("Buffer resource doesn't exist.");
}


}

