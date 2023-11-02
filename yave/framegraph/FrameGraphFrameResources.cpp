/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/buffers/Buffer.h>
#include <yave/graphics/device/DeviceProperties.h>
#include <yave/graphics/device/extensions/DebugUtils.h>

#include <y/utils/memory.h>

namespace yave {

FrameGraphFrameResources::FrameGraphFrameResources(std::shared_ptr<FrameGraphResourcePool> pool) : _pool(pool) {
}

FrameGraphFrameResources::~FrameGraphFrameResources() {
    for(auto&& res : _image_storage) {
        _pool->release(std::move(res.first), res.second);
    }
    for(auto&& res : _volume_storage) {
        _pool->release(std::move(res.first), res.second);
    }
    for(auto&& res : _buffer_storage) {
        _pool->release(std::move(res.first), res.second);
    }
    _pool->garbage_collect();
}

u64 FrameGraphFrameResources::frame_id() const {
    return _pool->frame_id();
}

u32 FrameGraphFrameResources::create_image_id() {
    return _next_image_id++;
}

u32 FrameGraphFrameResources::create_volume_id() {
    return _next_volume_id++;
}

u32 FrameGraphFrameResources::create_buffer_id() {
    return _next_buffer_id++;
}

void FrameGraphFrameResources::init_staging_buffer() {
    if(_staging_buffer_len) {
        _staging_buffer = StagingBuffer(_staging_buffer_len);

#ifdef Y_DEBUG
        if(const auto* debug = debug_utils()) {
            debug->set_resource_name(_staging_buffer.vk_buffer(), "Frame graph staging buffer");
        }
#endif
    }
}

void FrameGraphFrameResources::create_image(FrameGraphImageId res, TransientImage&& image, FrameGraphPersistentResourceId persistent_id) {
    res.check_valid();
    y_debug_assert(!image.is_null());

    auto* ptr = &_image_storage.emplace_back(std::move(image), persistent_id).first;

    _images.set_min_size(res.id() + 1);
    y_always_assert(!_images[res.id()], "Image already exists");
    _images[res.id()] = ptr;
}

void FrameGraphFrameResources::create_volume(FrameGraphVolumeId res, TransientVolume&& volume, FrameGraphPersistentResourceId persistent_id) {
    res.check_valid();
    y_debug_assert(!volume.is_null());

    auto* ptr = &_volume_storage.emplace_back(std::move(volume), persistent_id).first;

    _volumes.set_min_size(res.id() + 1);
    y_always_assert(!_volumes[res.id()], "Volume already exists");
    _volumes[res.id()] = ptr;
}

FrameGraphFrameResources::BufferData& FrameGraphFrameResources::create_buffer(FrameGraphBufferId res, TransientBuffer&& buffer, FrameGraphPersistentResourceId persistent_id) {
    res.check_valid();
    y_debug_assert(!buffer.is_null());

    auto* ptr = &_buffer_storage.emplace_back(std::move(buffer), persistent_id).first;

    _buffers.set_min_size(res.id() + 1);
    y_always_assert(!_buffers[res.id()].buffer, "Buffer already exists");
    _buffers[res.id()].buffer = ptr;

    return _buffers[res.id()];
}

void FrameGraphFrameResources::create_image(FrameGraphImageId res, ImageFormat format, const math::Vec2ui& size, ImageUsage usage, FrameGraphPersistentResourceId persistent_id) {
    create_image(res, _pool->create_image(format, size, usage), persistent_id);
}

void FrameGraphFrameResources::create_volume(FrameGraphVolumeId res, ImageFormat format, const math::Vec3ui& size, ImageUsage usage, FrameGraphPersistentResourceId persistent_id) {
    create_volume(res, _pool->create_volume(format, size, usage), persistent_id);
}

void FrameGraphFrameResources::create_buffer(FrameGraphBufferId res, u64 byte_size, BufferUsage usage, MemoryType memory, FrameGraphPersistentResourceId persistent_id) {
    BufferData& buffer = create_buffer(res, _pool->create_buffer(byte_size, usage, MemoryType::DeviceLocal), persistent_id);

    if(is_cpu_visible(memory)) {
        y_debug_assert(_staging_buffer.is_null());
        buffer.staging_buffer_offset = _staging_buffer_len;
        _staging_buffer_len += align_up_to(byte_size, device_properties().non_coherent_atom_size);
    }
}

void FrameGraphFrameResources::create_prev_image(FrameGraphImageId res, FrameGraphPersistentResourceId persistent_id) {
    persistent_id.check_valid();
    create_image(res, _pool->persistent_image(persistent_id), persistent_id);
}

void FrameGraphFrameResources::create_prev_buffer(FrameGraphBufferId res, FrameGraphPersistentResourceId persistent_id) {
    persistent_id.check_valid();
    create_buffer(res, _pool->persistent_buffer(persistent_id), persistent_id);
}

bool FrameGraphFrameResources::has_prev_image(FrameGraphPersistentResourceId persistent_id) const {
    return _pool->has_persistent_image(persistent_id);
}

bool FrameGraphFrameResources::has_prev_buffer(FrameGraphPersistentResourceId persistent_id) const {
    return _pool->has_persistent_buffer(persistent_id);
}

bool FrameGraphFrameResources::is_alive(FrameGraphImageId res) const {
    return res.id() < _images.size() && _images[res.id()] != nullptr;
}

bool FrameGraphFrameResources::is_alive(FrameGraphVolumeId res) const {
    return res.id() < _volumes.size() && _volumes[res.id()] != nullptr;
}

bool FrameGraphFrameResources::is_alive(FrameGraphBufferId res) const {
    return res.id() < _buffers.size() && _buffers[res.id()].buffer != nullptr;
}

ImageBarrier FrameGraphFrameResources::barrier(FrameGraphImageId res, PipelineStage src, PipelineStage dst) const {
    res.check_valid();
    return ImageBarrier(*_images[res.id()], src, dst);
}

ImageBarrier FrameGraphFrameResources::barrier(FrameGraphVolumeId res, PipelineStage src, PipelineStage dst) const {
    res.check_valid();
    return ImageBarrier(*_volumes[res.id()], src, dst);
}

BufferBarrier FrameGraphFrameResources::barrier(FrameGraphBufferId res, PipelineStage src, PipelineStage dst) const {
    res.check_valid();
    return BufferBarrier(*_buffers[res.id()].buffer, src, dst);
}

const ImageBase& FrameGraphFrameResources::image_base(FrameGraphImageId res) const {
    return find(res);
}

const ImageBase& FrameGraphFrameResources::volume_base(FrameGraphVolumeId res) const {
    return find(res);
}

const BufferBase& FrameGraphFrameResources::buffer_base(FrameGraphBufferId res) const {
    return find(res);
}

BufferMapping<u8> FrameGraphFrameResources::map_buffer_bytes(FrameGraphMutableBufferId res, MappingAccess access) const {
    return staging_buffer(res).map_bytes(access);
}

void FrameGraphFrameResources::flush_mapped_buffers(TransferCmdBufferRecorder& recorder) {
    y_profile();
    const auto region = recorder.region("Flush buffers");

    for(const auto& buffer : _buffers) {
        if(buffer.buffer && buffer.is_mapped()) {
            recorder.unbarriered_copy(staging_buffer(buffer), TransientSubBuffer<BufferUsage::TransferDstBit>(*buffer.buffer));
        }
    }
}

void FrameGraphFrameResources::create_alias(FrameGraphImageId dst, FrameGraphImageId src) {
    dst.check_valid();
    src.check_valid();

    TransientImage* orig = _images[src.id()];
    y_always_assert(orig, "Source image doesn't exists");

    _images.set_min_size(dst.id() + 1);

    TransientImage*& image = _images[dst.id()];
    y_always_assert(!image, "Destination image already exists");

    image = orig;
}

bool FrameGraphFrameResources::are_aliased(FrameGraphImageId a, FrameGraphImageId b) const {
    return &find(a) == &find(b);
}







const TransientImage& FrameGraphFrameResources::find(FrameGraphImageId res) const {
    y_always_assert(res.is_valid(), "Invalid image resource");

    if(!is_alive(res)) {
        y_fatal("Image resource doesn't exist");
    }

    return *_images[res.id()];
}

const TransientVolume& FrameGraphFrameResources::find(FrameGraphVolumeId res) const {
    y_always_assert(res.is_valid(), "Invalid volume resource");

    if(!is_alive(res)) {
        y_fatal("Volume resource doesn't exist");
    }

    return *_volumes[res.id()];
}

const TransientBuffer& FrameGraphFrameResources::find(FrameGraphBufferId res) const {
    y_always_assert(res.is_valid(), "Invalid buffer resource");

    if(!is_alive(res)) {
        y_fatal("Buffer resource doesn't exist");
    }

    return *_buffers[res.id()].buffer;
}

StagingSubBuffer FrameGraphFrameResources::staging_buffer(FrameGraphMutableBufferId res) const {
    y_always_assert(res.is_valid(), "Invalid buffer resource");
    y_always_assert(!_staging_buffer.is_null(), "Staging buffer has not been initialized");

    if(!is_alive(res)) {
        y_fatal("Buffer resource doesn't exist");
    }

    const auto buffer = _buffers[res.id()];
    if(!buffer.is_mapped()) {
        y_fatal("Buffer has not been mapped");
    }

    return staging_buffer(buffer);
}


StagingSubBuffer FrameGraphFrameResources::staging_buffer(const BufferData& buffer) const {
    y_debug_assert(buffer.is_mapped());
    return StagingSubBuffer(_staging_buffer, buffer.buffer->byte_size(), buffer.staging_buffer_offset);
}


}

