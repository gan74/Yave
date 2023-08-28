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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPHRESOURCEPOOL_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPHRESOURCEPOOL_H

#include "TransientBuffer.h"
#include "TransientImage.h"

#include <y/core/Vector.h>
#include <y/concurrent/Mutexed.h>

#include <atomic>

namespace yave {

class FrameGraphResourcePool : NonMovable {

    public:
        FrameGraphResourcePool();
        ~FrameGraphResourcePool();


        TransientImage create_image(ImageFormat format, const math::Vec2ui& size, ImageUsage usage);
        TransientVolume create_volume(ImageFormat format, const math::Vec3ui& size, ImageUsage usage);
        TransientBuffer create_buffer(u64 byte_size, BufferUsage usage, MemoryType memory);

        void release(TransientImage image);
        void release(TransientVolume volume);
        void release(TransientBuffer buffer);

        void garbage_collect();

    private:
        bool create_image_from_pool(TransientImage& res, ImageFormat format, const math::Vec2ui& size, ImageUsage usage);
        bool create_volume_from_pool(TransientVolume& res, ImageFormat format, const math::Vec3ui& size, ImageUsage usage);
        bool create_buffer_from_pool(TransientBuffer& res, usize byte_size, BufferUsage usage, MemoryType memory);

        // That's a lot of locking....
        concurrent::Mutexed<core::Vector<std::pair<TransientImage, u64>>, std::recursive_mutex> _images;
        concurrent::Mutexed<core::Vector<std::pair<TransientVolume, u64>>, std::recursive_mutex> _volumes;
        concurrent::Mutexed<core::Vector<std::pair<TransientBuffer, u64>>, std::recursive_mutex> _buffers;

        std::atomic<u64> _collection_id = 0;
};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHRESOURCEPOOL_H
