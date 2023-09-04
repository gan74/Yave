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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPHFRAMERESOURCES_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPHFRAMERESOURCES_H

#include "FrameGraphResourceId.h"
#include "TransientImage.h"
#include "TransientBuffer.h"

#include <yave/graphics/barriers/Barrier.h>
#include <yave/graphics/buffers/Buffer.h>

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
        ImageBarrier barrier(FrameGraphVolumeId res, PipelineStage src, PipelineStage dst) const;
        BufferBarrier barrier(FrameGraphBufferId res, PipelineStage src, PipelineStage dst) const;

        const ImageBase& image_base(FrameGraphImageId res) const;
        const ImageBase& volume_base(FrameGraphVolumeId res) const;
        const BufferBase& buffer_base(FrameGraphBufferId res) const;

        BufferMapping<u8> map_buffer_bytes(FrameGraphMutableBufferId res, MappingAccess access = MappingAccess::WriteOnly) const;

        template<ImageUsage Usage>
        ImageView<Usage> image(FrameGraphImageId res) const {
            return TransientImageView<Usage>(find(res));
        }

        template<ImageUsage Usage>
        ImageView<Usage, ImageType::ThreeD> volume(FrameGraphVolumeId res) const {
            return TransientImageView<Usage, ImageType::ThreeD>(find(res));
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
        BufferMapping<T> map_buffer(FrameGraphMutableTypedBufferId<T> res, MappingAccess access = MappingAccess::WriteOnly) const {
            const TypedSubBuffer<T, StagingBuffer::usage, StagingBuffer::memory_type> sub_buffer(staging_buffer(res));
            return sub_buffer.map(access);
        }



    private:
        friend class FrameGraph;

        void init_staging_buffer();

        u32 create_image_id();
        u32 create_volume_id();
        u32 create_buffer_id();

        void create_image(FrameGraphImageId res, ImageFormat format, const math::Vec2ui& size, ImageUsage usage, FrameGraphPersistentResourceId persistent_id = {});
        void create_volume(FrameGraphVolumeId res, ImageFormat format, const math::Vec3ui& size, ImageUsage usage, FrameGraphPersistentResourceId persistent_id = {});
        void create_buffer(FrameGraphBufferId res, u64 byte_size, BufferUsage usage, MemoryType memory, FrameGraphPersistentResourceId persistent_id = {});

        void create_prev_image(FrameGraphImageId res, FrameGraphPersistentResourceId persistent_id);
        void create_prev_buffer(FrameGraphBufferId res, FrameGraphPersistentResourceId persistent_id);

        bool has_prev_image(FrameGraphPersistentResourceId persistent_id) const;
        bool has_prev_buffer(FrameGraphPersistentResourceId persistent_id) const;

        bool is_alive(FrameGraphImageId res) const;
        bool is_alive(FrameGraphVolumeId res) const;
        bool is_alive(FrameGraphBufferId res) const;

        void create_alias(FrameGraphImageId dst, FrameGraphImageId src);

        void flush_mapped_buffers(CmdBufferRecorder& recorder);

        const TransientImage& find(FrameGraphImageId res) const;
        const TransientVolume& find(FrameGraphVolumeId res) const;
        const TransientBuffer& find(FrameGraphBufferId res) const;

        void create_image(FrameGraphImageId res, TransientImage&& image, FrameGraphPersistentResourceId persistent_id);
        void create_volume(FrameGraphVolumeId res, TransientVolume&& volume, FrameGraphPersistentResourceId persistent_id);
        BufferData& create_buffer(FrameGraphBufferId res, TransientBuffer&& buffer, FrameGraphPersistentResourceId persistent_id);

        StagingSubBuffer staging_buffer(FrameGraphMutableBufferId res) const;
        StagingSubBuffer staging_buffer(const BufferData& buffer) const;

        u32 _next_image_id = 0;
        u32 _next_volume_id = 0;
        u32 _next_buffer_id = 0;

        core::Vector<TransientImage*> _images;
        core::Vector<TransientVolume*> _volumes;
        core::Vector<BufferData> _buffers;

        std::shared_ptr<FrameGraphResourcePool> _pool;

        // We need pointer stability
        std::deque<std::pair<TransientImage, FrameGraphPersistentResourceId>> _image_storage;
        std::deque<std::pair<TransientVolume, FrameGraphPersistentResourceId>> _volume_storage;
        std::deque<std::pair<TransientBuffer, FrameGraphPersistentResourceId>> _buffer_storage;

        StagingBuffer _staging_buffer;
        u64 _staging_buffer_len = 0;
};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHFRAMERESOURCES_H

