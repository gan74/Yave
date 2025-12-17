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
#ifndef YAVE_SCENE_TRANSFORM_MANAGER_H
#define YAVE_SCENE_TRANSFORM_MANAGER_H

#include <y/math/Transform.h>

#include <yave/graphics/buffers/Buffer.h>
#include <yave/graphics/buffers/buffers.h>
#include <yave/graphics/shader_structs.h>

#include <yave/utils/IndexAllocator.h>

namespace yave {

class TransformManager : NonMovable {
    struct TransformData {
        math::Transform<> transform;

        bool is_dirty = true;
        bool to_reset = true;

#ifdef Y_DEBUG
        bool is_valid = false;
#endif
    };

    public:
        static constexpr BufferUsage buffer_usage = BufferUsage::StorageBit | BufferUsage::TransferDstBit | BufferUsage::TransferSrcBit;
        using TransformBuffer = TypedBuffer<shader::TransformableData, buffer_usage>;

        u32 alloc_transform();
        void free_transform(u32 index);
        void set_transform(u32 index, const math::Transform<>& tr);

        const math::Transform<>& transform(u32 index) const;

        bool need_update() const;
        void update_buffer(ComputeCapableCmdBufferRecorder& recorder);

        TypedSubBuffer<shader::TransformableData, BufferUsage::StorageBit> transform_buffer() const  {
            return _transform_buffer;
        }


    private:
        TransformBuffer _transform_buffer;

        IndexAllocator<u32> _index_allocator;

        core::Vector<TransformData> _transforms;
        ProfiledMutexed<core::Vector<u32>> _dirty;

};


}

#endif // YAVE_SCENE_TRANSFORM_MANAGER_H

