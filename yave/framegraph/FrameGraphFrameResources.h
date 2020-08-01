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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPHFRAMERESOURCES_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPHFRAMERESOURCES_H

#include "FrameGraphResourceId.h"
#include "TransientImage.h"
#include "TransientBuffer.h"

#include <yave/graphics/barriers/Barrier.h>

#include <y/core/Vector.h>

#include <unordered_map>
#include <memory>

namespace yave {

class FrameGraphFrameResources final : NonMovable {
    public:
        FrameGraphFrameResources(std::shared_ptr<FrameGraphResourcePool> pool);
        ~FrameGraphFrameResources();

        DevicePtr device() const;

        u32 create_resource_id();

        void create_image(FrameGraphImageId res, ImageFormat format, const math::Vec2ui& size, ImageUsage usage);
        void create_buffer(FrameGraphBufferId res, usize byte_size, BufferUsage usage, MemoryType memory);

        bool is_alive(FrameGraphImageId res) const;
        bool is_alive(FrameGraphBufferId res) const;

        ImageBarrier barrier(FrameGraphImageId res, PipelineStage src, PipelineStage dst) const;
        BufferBarrier barrier(FrameGraphBufferId res, PipelineStage src, PipelineStage dst) const;

        const ImageBase& image_base(FrameGraphImageId res) const;
        const BufferBase& buffer_base(FrameGraphBufferId res) const;


        void create_alias(FrameGraphImageId dst, FrameGraphImageId src);
        bool are_aliased(FrameGraphImageId a, FrameGraphImageId b) const;



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
        TypedMapping<T> mapped_buffer(FrameGraphMutableTypedBufferId<T> res) const {
            constexpr BufferUsage usage = BufferUsage::None;
            constexpr MemoryType memory = MemoryType::CpuVisible;
            const TypedSubBuffer<T, usage, memory> subbuffer(TransientSubBuffer<usage, memory>(find(res)));
            return TypedMapping<T>(subbuffer);
        }

    private:
        const TransientImage<>& find(FrameGraphImageId res) const;
        const TransientBuffer& find(FrameGraphBufferId res) const;

        u32 _next_id = 0;

        Y_TODO(replace by vector)
        using hash_t = std::hash<FrameGraphResourceId>;
        std::unordered_map<FrameGraphImageId, TransientImage<>*, hash_t> _images;
        std::unordered_map<FrameGraphBufferId, TransientBuffer*, hash_t> _buffers;

        std::shared_ptr<FrameGraphResourcePool> _pool;

        core::Vector<std::unique_ptr<TransientImage<>>> _image_storage;
        core::Vector<std::unique_ptr<TransientBuffer>> _buffer_storage;
};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHFRAMERESOURCES_H

