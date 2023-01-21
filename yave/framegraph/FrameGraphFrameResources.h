/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPHFRAMERESOURCES_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPHFRAMERESOURCES_H

#include "FrameGraphResourceId.h"
#include "TransientImage.h"
#include "TransientBuffer.h"

#include <yave/graphics/barriers/Barrier.h>
#include <yave/graphics/buffers/buffers.h>

#include <y/core/Vector.h>
#include <y/core/HashMap.h>

#include <deque>

namespace yave {

class FrameGraphFrameResources final : NonMovable {
    struct BufferData {
        TransientBuffer* buffer = nullptr;
        u64 staging_buffer_offset = u64(-1);

        bool is_mapped() const {
            return staging_buffer_offset != u64(-1);
        }
    };

    public:
        FrameGraphFrameResources(std::shared_ptr<FrameGraphResourcePool> pool);
        ~FrameGraphFrameResources();

        bool are_aliased(FrameGraphImageId a, FrameGraphImageId b) const;

        ImageBarrier barrier(FrameGraphImageId res, PipelineStage src, PipelineStage dst) const;
        BufferBarrier barrier(FrameGraphBufferId res, PipelineStage src, PipelineStage dst) const;

        const ImageBase& image_base(FrameGraphImageId res) const;
        const BufferBase& buffer_base(FrameGraphBufferId res) const;


        template<ImageUsage Usage>
        ImageView<Usage> image(FrameGraphImageId res) const {
            return TransientImageView<Usage>(find(res));
        }

        template<BufferUsage Usage>
        SubBuffer<Usage> buffer(FrameGraphBufferId res) const {
            return TransientSubBuffer<Usage>(find(res));
        }

        template<BufferUsage Usage, typename T>
        TypedSubBuffer<T, Usage> buffer(FrameGraphTypedBufferId<T> res) const {
            return TypedSubBuffer<T, Usage>(TransientSubBuffer<Usage>(find(res)));
        }

        template<typename T>
        TypedMapping<T> map_buffer(FrameGraphMutableTypedBufferId<T> res) const {
            constexpr BufferUsage usage = StagingBuffer::usage;
            constexpr MemoryType memory = StagingBuffer::memory_type;
            const TypedSubBuffer<T, usage, memory> sub_buffer(staging_buffer(res));
            return TypedMapping<T>(sub_buffer);
        }

    private:
        friend class FrameGraph;

        void reserve(usize images, usize buffers);
        void init_staging_buffer();

        u32 create_image_id();
        u32 create_buffer_id();

        void create_image(FrameGraphImageId res, ImageFormat format, const math::Vec2ui& size, ImageUsage usage);
        void create_buffer(FrameGraphBufferId res, u64 byte_size, BufferUsage usage, MemoryType memory);

        bool is_alive(FrameGraphImageId res) const;
        bool is_alive(FrameGraphBufferId res) const;

        void create_alias(FrameGraphImageId dst, FrameGraphImageId src);

        void flush_mapped_buffers(CmdBufferRecorder& recorder);

    private:
        const TransientImage<>& find(FrameGraphImageId res) const;
        const TransientBuffer& find(FrameGraphBufferId res) const;

        StagingSubBuffer staging_buffer(FrameGraphMutableBufferId res) const;
        StagingSubBuffer staging_buffer(const BufferData& buffer) const;

        u32 _next_image_id = 0;
        u32 _next_buffer_id = 0;

        core::Vector<TransientImage<>*> _images;
        core::Vector<BufferData> _buffers;

        std::shared_ptr<FrameGraphResourcePool> _pool;

        // We need pointer stability
        std::deque<TransientImage<>> _image_storage;
        std::deque<TransientBuffer> _buffer_storage;

        StagingBuffer _staging_buffer;
        u64 _staging_buffer_len = 0;
};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHFRAMERESOURCES_H

