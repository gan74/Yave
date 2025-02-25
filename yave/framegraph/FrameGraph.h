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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPH_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPH_H

#include "FrameGraphPassBuilder.h"

#include <y/core/Vector.h>
#include <y/core/String.h>
#include <y/core/HashMap.h>
#include <y/core/FixedArray.h>

#include <memory>

namespace yave {

class FrameGraphRegion : NonMovable {
    public:
        ~FrameGraphRegion();

    private:
        friend class FrameGraph;

        FrameGraphRegion(FrameGraph* parent, usize index);

        FrameGraph* _parent = nullptr;
        usize _index = 0;
};

class FrameGraph : NonMovable {

    struct Region {
        core::String name;
        usize begin_pass = 0;
        usize end_pass = 0;
    };

    struct ResourceCreateInfo {
        usize last_read = 0;
        usize last_write = 0;
        usize first_use = 0;

        core::SmallVector<FrameGraphPersistentResourceId, 4> persistents;  // Persistent
        bool is_prev = false;

        bool is_persistent() const;
        usize last_use() const;
        void register_use(usize index, bool is_written);
    };

    struct ImageCreateInfo : ResourceCreateInfo {
        math::Vec3ui size;
        u32 mips = 1;
        ImageFormat format;
        ImageUsage usage = ImageUsage::None;
        ImageUsage last_usage = ImageUsage::None;

        FrameGraphImageId copy_src;
        FrameGraphImageId alias;

        void register_alias(const ImageCreateInfo& other);
        bool is_aliased() const;
        bool has_usage() const;
    };

    struct BufferCreateInfo : ResourceCreateInfo {
        u64 byte_size = 0;
        BufferUsage usage = BufferUsage::None;
        MemoryType memory_type = MemoryType::DontCare;
    };

    struct ImageClearInfo {
        usize pass_index = 0;
        FrameGraphMutableImageId dst;
    };

    struct ImageCopyInfo {
        usize pass_index = 0;
        FrameGraphMutableImageId dst;
        FrameGraphImageId src;
    };

    struct BufferCopyInfo {
        usize pass_index = 0;
        FrameGraphMutableBufferId dst;
        FrameGraphBufferId src;
    };

    struct InlineStorage {
        InlineStorage(usize size) : storage(size) {}

        core::FixedArray<u32> storage;
        usize used = 0;
    };

    static constexpr bool allow_image_aliasing = true;

    public:
        FrameGraph(std::shared_ptr<FrameGraphResourcePool> pool);
        ~FrameGraph();

        u64 frame_id() const;
        const FrameGraphFrameResources& resources() const;

        FrameGraphRegion region(std::string_view name);

        void render(CmdBufferRecorder& recorder, CmdTimestampPool* ts_pool = nullptr);

        FrameGraphPassBuilder add_pass(std::string_view name);
        FrameGraphComputePassBuilder add_compute_pass(std::string_view name);

        FrameGraphImageId make_persistent_and_get_prev(FrameGraphImageId res, FrameGraphPersistentResourceId persistent_id);
        FrameGraphBufferId make_persistent_and_get_prev(FrameGraphBufferId res, FrameGraphPersistentResourceId persistent_id);

        math::Vec2ui image_size(FrameGraphImageId res) const;
        math::Vec3ui volume_size(FrameGraphVolumeId res) const;

        ImageFormat image_format(FrameGraphImageId res) const;
        ImageFormat volume_format(FrameGraphVolumeId res) const;

        u64 buffer_byte_size(FrameGraphBufferId res) const;

        template<typename T>
        usize buffer_size(FrameGraphTypedBufferId<T> res) const {
            return usize(buffer_byte_size(res) / sizeof(T));
        }

        template<typename T>
        usize buffer_size(FrameGraphMutableTypedBufferId<T> res) const {
            return usize(buffer_byte_size(res) / sizeof(T));
        }

    private:
        friend class FrameGraphPassBuilderBase;
        friend class FrameGraphPass;
        friend class FrameGraphRegion;

        void end_region(usize index);

        FrameGraphMutableImageId declare_image(ImageFormat format, const math::Vec2ui& size, u32 mips);
        FrameGraphMutableVolumeId declare_volume(ImageFormat format, const math::Vec3ui& size);
        FrameGraphMutableBufferId declare_buffer(u64 byte_size);

        const ImageCreateInfo& info(FrameGraphImageId res) const;
        const ImageCreateInfo& info(FrameGraphVolumeId res) const;
        const BufferCreateInfo& info(FrameGraphBufferId res) const;


        void register_usage(FrameGraphImageId res, ImageUsage usage, bool is_written, const FrameGraphPass* pass);
        void register_usage(FrameGraphVolumeId res, ImageUsage usage, bool is_written, const FrameGraphPass* pass);
        void register_usage(FrameGraphBufferId res, BufferUsage usage, bool is_written, const FrameGraphPass* pass);

        void register_image_copy(FrameGraphMutableImageId dst, FrameGraphImageId src, const FrameGraphPass* pass);
        void register_buffer_copy(FrameGraphMutableBufferId dst, FrameGraphBufferId src, const FrameGraphPass* pass);

        void register_image_clear(FrameGraphMutableImageId dst, const FrameGraphPass* pass);

        [[nodiscard]] InlineDescriptor copy_inline_descriptor(InlineDescriptor desc);

        void map_buffer(FrameGraphMutableBufferId res, const FrameGraphPass* pass);

        bool is_attachment(FrameGraphImageId res) const;

    private:
        const core::String& pass_name(usize pass_index) const;

        FrameGraphPass* create_pass(std::string_view name);

        void alloc_resources();
        void alloc_image(FrameGraphImageId res, const ImageCreateInfo& info) const;

        std::unique_ptr<FrameGraphFrameResources> _resources;

        core::Vector<std::unique_ptr<FrameGraphPass>> _passes;

        core::Vector<std::pair<FrameGraphMutableImageId, ImageCreateInfo>> _images;
        core::Vector<std::pair<FrameGraphMutableVolumeId, ImageCreateInfo>> _volumes;
        core::Vector<std::pair<FrameGraphMutableBufferId, BufferCreateInfo>> _buffers;

        core::Vector<ImageCopyInfo> _image_copies;
        core::Vector<BufferCopyInfo> _buffer_copies;

        core::Vector<ImageClearInfo> _image_clears;

        core::Vector<InlineStorage> _inline_storage;

        usize _pass_index = 0;

        core::Vector<Region> _regions;

        core::Vector<FrameGraphResourceId> _persistents;

};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPH_H

