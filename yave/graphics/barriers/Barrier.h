/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_BARRIERS_BARRIER_H
#define YAVE_GRAPHICS_BARRIERS_BARRIER_H

#include "PipelineStage.h"

#include <yave/graphics/images/ImageView.h>
#include <yave/graphics/buffers/Buffer.h>
#include <yave/graphics/buffers/SubBuffer.h>

#include <y/core/Result.h>

namespace yave {

class ImageBarrier {
    public:
        ImageBarrier(const ImageBase& image, PipelineStage src, PipelineStage dst);

        // for internal use, don't call for giggles
        static ImageBarrier transition_barrier(const ImageBase& image, VkImageLayout src_layout, VkImageLayout dst_layout);
        static ImageBarrier transition_to_barrier(const ImageBase& image, VkImageLayout dst_layout);
        static ImageBarrier transition_from_barrier(const ImageBase& image, VkImageLayout src_layout);


        VkImageMemoryBarrier vk_barrier() const;

        PipelineStage dst_stage() const;
        PipelineStage src_stage() const;

    private:
        ImageBarrier() = default;

        VkImageMemoryBarrier _barrier;
        PipelineStage _src;
        PipelineStage _dst;
};

class BufferBarrier {
    public:
        BufferBarrier(const BufferBase& buffer, PipelineStage src, PipelineStage dst);
        BufferBarrier(const SubBufferBase& buffer, PipelineStage src, PipelineStage dst);


        VkBufferMemoryBarrier vk_barrier() const;

        PipelineStage dst_stage() const;
        PipelineStage src_stage() const;

    private:
        VkBufferMemoryBarrier _barrier;
        PipelineStage _src;
        PipelineStage _dst;
};

}


#endif // YAVE_GRAPHICS_BARRIERS_BARRIER_H

