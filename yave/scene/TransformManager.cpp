/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include "TransformManager.h"

#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/barriers/Barrier.h>
#include <yave/graphics/device/DeviceResources.h>

namespace yave {

u32 TransformManager::alloc_transform() {
    const u32 index = _index_allocator.alloc();

    _transforms.set_min_size(index + 1);
    _dirty << index;

    y_debug_assert(!std::exchange(_transforms[index].is_valid, true));

    return index;
}

void TransformManager::free_transform(u32 index) {
    y_debug_assert(std::exchange(_transforms[index].is_valid, false));

    _index_allocator.free(index);
}

void TransformManager::set_transform(u32 index, const math::Transform<>& tr) {
    y_debug_assert(_transforms[index].is_valid);

    auto& data = _transforms[index];

    if(!data.is_dirty) {
        if(data.to_reset) {
            data.to_reset = false;
        } else {
            _dirty << index;
        }
        data.is_dirty = true;
    }

    data.transform = tr;
}

const math::Transform<>& TransformManager::transform(u32 index) const {
    y_debug_assert(index != u32(-1));
    y_debug_assert(_transforms[index].is_valid);

    return _transforms[index].transform;
}


bool TransformManager::need_update() const {
    return !_dirty.is_empty();
}

void TransformManager::update_buffer(ComputeCapableCmdBufferRecorder& recorder) {
    y_profile();

    const usize update_count = _dirty.size();
    if(!update_count) {
        return;
    }

    TransformBuffer new_buffer;
    if(_transforms.size() > _transform_buffer.size()) {
        const usize new_base_size = 2_uu << log2ui(_transforms.size());
        y_debug_assert(new_base_size > _transform_buffer.size());
        new_buffer = TransformBuffer(new_base_size);
    };


    auto transform_staging = TypedBuffer<math::Transform<>, BufferUsage::StorageBit, MemoryType::Staging>(update_count);
    auto index_staging = TypedBuffer<u32, BufferUsage::StorageBit, MemoryType::Staging>(update_count);

    {
        usize updates = 0;
        auto transform_mapping = transform_staging.map(MappingAccess::WriteOnly);
        auto index_mapping = index_staging.map(MappingAccess::WriteOnly);

        for(const u32 index : _dirty) {
            y_debug_assert(_transforms[index].is_valid);

            const auto& data = _transforms[index];
            transform_mapping[updates] = data.transform;
            index_mapping[updates] = (index | (data.to_reset ? 0x80000000 : 0x00000000)); // Noop on little endian
            ++updates;
        }

        y_debug_assert(updates == update_count);
    }

    {
        const auto region = recorder.region("Transform update");
        if(!new_buffer.is_null()) {
            if(!_transform_buffer.is_null()) {
                recorder.unbarriered_copy(_transform_buffer, SubBuffer<BufferUsage::TransferDstBit>(new_buffer, _transform_buffer.byte_size(), 0));
                recorder.barriers(BufferBarrier(new_buffer, PipelineStage::TransferBit, PipelineStage::ComputeBit));
            }
            std::swap(_transform_buffer, new_buffer);
        }

        const auto descriptors = make_descriptor_set(
            _transform_buffer,
            transform_staging,
            index_staging,
            InlineDescriptor(u32(update_count))
        );

        const auto& program = device_resources()[DeviceResources::UpdateTransformsProgram];
        recorder.dispatch_threads(program, math::Vec2ui(u32(update_count), 1), DescriptorSetProxy(descriptors));
    }


    auto next_dirty = core::Vector<u32>::with_capacity(_dirty.size());
    for(const u32 index : _dirty) {
        y_debug_assert(_transforms[index].is_valid);

        auto& data = _transforms[index];
        if(!data.to_reset) {
            data.to_reset = true;
            data.is_dirty = false;
            next_dirty << index;
        } else {
            data.to_reset = false;
            data.is_dirty = false;
        }
    }

    _dirty.swap(next_dirty);
}

}

