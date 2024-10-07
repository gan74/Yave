/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#include "Framebuffer.h"
#include "RenderPass.h"
#include <yave/graphics/graphics.h>

#include <y/core/ScratchPad.h>

namespace yave {

static math::Vec2ui compute_size(const Framebuffer::DepthAttachment& depth, core::Span<Framebuffer::ColorAttachment> colors) {
    const math::Vec2ui ref_size = depth.view.is_null()
        ? (colors.is_empty() ? math::Vec2ui{} : colors[0].view.size())
        : depth.view.size();

    for(const auto& c : colors) {
        y_always_assert(c.view.size() == ref_size, "Invalid attachment size");
    }

    return ref_size;
}

static core::ScratchPad<Framebuffer::ColorAttachment> color_attachments(core::Span<ColorAttachmentView> color_views, Framebuffer::LoadOp load_op) {
    auto colors = core::ScratchPad<Framebuffer::ColorAttachment>(color_views.size());
    std::transform(color_views.begin(), color_views.end(), colors.begin(), [=](const auto& c) { return Framebuffer::ColorAttachment(c, load_op); });
    return colors;
}

static RenderPass create_render_pass(const Framebuffer::DepthAttachment& depth, core::Span<Framebuffer::ColorAttachment> colors) {
    auto color_vec = core::ScratchPad<RenderPass::AttachmentData>(colors.size());
    std::transform(colors.begin(), colors.end(), color_vec.begin(), [](const auto& c) { return RenderPass::AttachmentData(c.view, c.load_op); });
    return !depth.view.is_null()
        ? RenderPass(RenderPass::AttachmentData(depth.view, depth.load_op), color_vec)
        : RenderPass(color_vec);
}

Framebuffer::Framebuffer(const DepthAttachment& depth, core::Span<ColorAttachment> colors) :
        _size(compute_size(depth, colors)),
        _attachment_count(colors.size()),
        _render_pass(create_render_pass(depth, colors)) {

    const bool has_depth = !depth.view.is_null();

    auto views = core::ScratchPad<VkImageView>(colors.size() + has_depth);
    std::transform(colors.begin(), colors.end(), views.begin(), [](const auto& v) { return v.view.vk_view(); });

    if(has_depth) {
        views.last() = depth.view.vk_view();
    }

    VkFramebufferCreateInfo create_info = vk_struct();
    {
        create_info.renderPass = _render_pass.vk_render_pass();
        create_info.attachmentCount = u32(views.size());
        create_info.pAttachments = views.data();
        create_info.width = _size.x();
        create_info.height = _size.y();
        create_info.layers = 1;
    }

    vk_check(vkCreateFramebuffer(vk_device(), &create_info, vk_allocation_callbacks(), _framebuffer.get_ptr_for_init()));
}

Framebuffer::Framebuffer(core::Span<ColorAttachmentView> colors, LoadOp load_op) :
        Framebuffer(DepthAttachment(), color_attachments(colors, load_op)) {
}

Framebuffer::Framebuffer(const DepthAttachmentView& depth, core::Span<ColorAttachmentView> colors, LoadOp load_op) :
        Framebuffer(DepthAttachment(depth, load_op), color_attachments(colors, load_op)) {
}

Framebuffer::~Framebuffer() {
    destroy_graphic_resource(std::move(_framebuffer));
}

bool Framebuffer::is_null() const {
    return !_framebuffer;
}

VkFramebuffer Framebuffer::vk_framebuffer() const {
    y_debug_assert(!is_null());
    return _framebuffer;
}

const math::Vec2ui& Framebuffer::size() const {
    return _size;
}

const RenderPass& Framebuffer::render_pass() const {
    return _render_pass;
}

usize Framebuffer::attachment_count() const {
    return _attachment_count;
}

}

