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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPHPASS_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPHPASS_H

#include "FrameGraphResourceId.h"
#include "FrameGraphDescriptorBinding.h"

#include <yave/graphics/descriptors/DescriptorSetCommon.h>
#include <yave/graphics/framebuffer/Framebuffer.h>
#include <yave/graphics/barriers/PipelineStage.h>

#include <y/core/String.h>
#include <y/core/HashMap.h>

namespace yave {

class FrameGraphPass final : NonMovable {

    struct Attachment {
        FrameGraphImageId image;
        u32 mip = 0;
    };

    public:
        struct ResourceUsageInfo {
            PipelineStage stage = PipelineStage::None;
            bool written_to = false;
        };

        using render_func = std::function<void(RenderPassRecorder&, const FrameGraphPass*)>;
        using compute_render_func = std::function<void(CmdBufferRecorder&, const FrameGraphPass*)>;

        FrameGraphPass(std::string_view name, FrameGraph* parent, usize index);

        const core::String& name() const;
        const FrameGraphFrameResources& resources() const;

        const Framebuffer& framebuffer() const;
        DescriptorSetCommon descriptor_set(usize index = 0) const;

        void render(CmdBufferRecorder& recorder);

    private:
        friend class FrameGraph;
        friend class FrameGraphPassBuilderBase;

        void init_framebuffer(const FrameGraphFrameResources& resources);
        void init_descriptor_sets(const FrameGraphFrameResources& resources);

        ResourceUsageInfo& info(FrameGraphImageId res);
        ResourceUsageInfo& info(FrameGraphVolumeId res);
        ResourceUsageInfo& info(FrameGraphBufferId res);

        render_func _render = nullptr;
        compute_render_func _compute_render = nullptr;

        core::String _name;

        FrameGraph* _parent = nullptr;
        const usize _index;

        using hash_t = std::hash<FrameGraphResourceId>;
        core::FlatHashMap<FrameGraphImageId, ResourceUsageInfo, hash_t> _images;
        core::FlatHashMap<FrameGraphVolumeId, ResourceUsageInfo, hash_t> _volumes;
        core::FlatHashMap<FrameGraphBufferId, ResourceUsageInfo, hash_t> _buffers;

        core::SmallVector<core::SmallVector<FrameGraphDescriptorBinding, 8>, 4> _bindings;
        core::SmallVector<core::SmallVector<Descriptor, 8>, 2> _descriptor_sets;

        core::SmallVector<std::pair<FrameGraphMutableBufferId, FrameGraphInlineBlock>, 4> _map_data;

        Attachment _depth;
        core::SmallVector<Attachment, 6> _colors;

        Framebuffer _framebuffer;
};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHPASS_H

