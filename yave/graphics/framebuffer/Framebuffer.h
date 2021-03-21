/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_FRAMEBUFFER_FRAMEBUFFER_H
#define YAVE_GRAPHICS_FRAMEBUFFER_FRAMEBUFFER_H

#include "RenderPass.h"

#include <yave/graphics/images/ImageView.h>

#include <memory>

namespace yave {

class Framebuffer final {

    public:
        using LoadOp = RenderPass::LoadOp;

        template<ImageUsage Usage>
        struct Attachment {
            Attachment() = default;

            Attachment(const ImageView<Usage>& v) : view(v) {
            }

            Attachment(const ImageView<Usage>& v, LoadOp op) : view(v), load_op(op) {
            }

            ImageView<Usage> view;
            LoadOp load_op = LoadOp::Clear;
        };

        using ColorAttachment = Attachment<ImageUsage::ColorBit>;
        using DepthAttachment = Attachment<ImageUsage::DepthBit>;

        Framebuffer() = default;
        Framebuffer(Framebuffer&&) = default;
        Framebuffer& operator=(Framebuffer&&) = default;

        Framebuffer(const DepthAttachment& depth, core::Span<ColorAttachment> colors = {});
        Framebuffer(core::Span<ColorAttachmentView> colors, LoadOp load_op = LoadOp::Clear);
        Framebuffer(const DepthAttachmentView& depth, core::Span<ColorAttachmentView> colors = {}, LoadOp load_op = LoadOp::Clear);

        ~Framebuffer();

        bool is_null() const;
        VkFramebuffer vk_framebuffer() const;

        const math::Vec2ui& size() const;

        const RenderPass& render_pass() const;

        usize attachment_count() const;

    private:
        math::Vec2ui _size;
        usize _attachment_count = 0;

        std::unique_ptr<RenderPass> _render_pass;
        VkHandle<VkFramebuffer> _framebuffer;
};

}

#endif // YAVE_GRAPHICS_FRAMEBUFFER_FRAMEBUFFER_H

