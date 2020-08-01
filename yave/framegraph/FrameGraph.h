/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPH_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPH_H

#include "FrameGraphPassBuilder.h"

#include <y/core/Vector.h>

#include <memory>

namespace yave {

class FrameGraph : NonCopyable {

    struct ResourceCreateInfo {
        usize last_read = 0;
        usize last_write = 0;
        usize first_use = 0;

        bool can_alias_on_last = false;

        usize last_use() const;
        void register_use(usize index, bool is_written);
    };

    struct ImageCreateInfo : ResourceCreateInfo {
        math::Vec2ui size;
        ImageFormat format;
        ImageUsage usage = ImageUsage::None;

        FrameGraphImageId copy_src;
        FrameGraphImageId alias;

        void register_alias(const ImageCreateInfo& other);
        bool is_aliased() const;
        bool has_usage() const;
    };

    struct BufferCreateInfo : ResourceCreateInfo {
        usize byte_size;
        BufferUsage usage = BufferUsage::None;
        MemoryType memory_type = MemoryType::DontCare;
    };

    struct ImageCopyInfo {
        usize pass_index = 0;
        FrameGraphMutableImageId dst;
        FrameGraphImageId src;
    };

    public:
        FrameGraph(std::shared_ptr<FrameGraphResourcePool> pool);
        ~FrameGraph();

        DevicePtr device() const;
        const FrameGraphFrameResources& resources() const;

        void render(CmdBufferRecorder& recorder) &&;

        FrameGraphPassBuilder add_pass(std::string_view name);

        math::Vec2ui image_size(FrameGraphImageId res) const;
        ImageFormat image_format(FrameGraphImageId res) const;

    private:
        friend class FrameGraphPassBuilder;
        friend class FrameGraphPass;

        FrameGraphMutableImageId declare_image(ImageFormat format, const math::Vec2ui& size);
        FrameGraphMutableBufferId declare_buffer(usize byte_size);

        const ImageCreateInfo& info(FrameGraphImageId res) const;
        const BufferCreateInfo& info(FrameGraphBufferId res) const;

        void register_usage(FrameGraphImageId res, ImageUsage usage, bool is_written, const FrameGraphPass* pass);
        void register_usage(FrameGraphBufferId res, BufferUsage usage, bool is_written, const FrameGraphPass* pass);
        void register_image_copy(FrameGraphMutableImageId dst, FrameGraphImageId src, const FrameGraphPass* pass);

        void set_cpu_visible(FrameGraphMutableBufferId res, const FrameGraphPass* pass);

        bool is_attachment(FrameGraphImageId res) const;

    private:
        const core::String& pass_name(usize pass_index) const;

        void alloc_resources();
        void alloc_image(FrameGraphImageId res, const ImageCreateInfo& info) const;

        std::unique_ptr<FrameGraphFrameResources> _resources;

        core::Vector<std::unique_ptr<FrameGraphPass>> _passes;

        using hash_t = std::hash<FrameGraphResourceId>;
        std::unordered_map<FrameGraphImageId, ImageCreateInfo, hash_t> _images;
        std::unordered_map<FrameGraphBufferId, BufferCreateInfo, hash_t> _buffers;

        core::Vector<ImageCopyInfo> _image_copies;

        usize _pass_index = 0;

};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPH_H

