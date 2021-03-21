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
#ifndef YAVE_GRAPHICS_FRAMEBUFFER_RENDERPASS_H
#define YAVE_GRAPHICS_FRAMEBUFFER_RENDERPASS_H

#include <yave/yave.h>

#include <yave/graphics/graphics.h>
#include <yave/graphics/graphics.h>
#include <yave/graphics/images/Image.h>
#include <yave/graphics/images/ImageView.h>

#include <y/core/Vector.h>

namespace yave {

class RenderPass {
    public:
        enum class LoadOp : u32 {
            Clear,
            Load
        };

        struct AttachmentData {
            const ImageFormat format;
            const ImageUsage usage = ImageUsage::None;
            const LoadOp load_op = LoadOp::Clear;

            AttachmentData() = default;

            AttachmentData(ImageFormat fmt, ImageUsage us, LoadOp op) : format(fmt), usage(us), load_op(op) {
            }

            AttachmentData(const ImageBase& img, LoadOp op) : format(img.format()), usage(img.usage()), load_op(op) {
            }

            template<ImageUsage Usage>
            AttachmentData(const ImageView<Usage>& img, LoadOp op) : format(img.format()), usage(img.usage()), load_op(op) {
            }
        };

        class Layout {
            public:
                Layout() = default;
                Layout(AttachmentData depth, core::Span<AttachmentData> colors);

                u64 hash() const;
                bool is_depth_only() const;

                bool operator==(const Layout& other) const;

            private:
                ImageFormat _depth;
                core::SmallVector<ImageFormat, 7> _colors;
        };

        RenderPass() = default;
        RenderPass(RenderPass&&) = default;
        RenderPass& operator=(RenderPass&&) = default;

        RenderPass(AttachmentData depth, core::Span<AttachmentData> colors);
        RenderPass(core::Span<AttachmentData> colors);

        ~RenderPass();

        bool is_depth_only() const;

        const Layout& layout() const;
        usize attachment_count() const;

        bool is_null() const;

        VkRenderPass vk_render_pass() const;

    private:
        usize _attachment_count = 0;
        VkHandle<VkRenderPass> _render_pass;

        Layout _layout;
};

}

#endif // YAVE_GRAPHICS_FRAMEBUFFER_RENDERPASS_H

