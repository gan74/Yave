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
#include "RenderPass.h"

#include <yave/graphics/images/Image.h>
#include <yave/graphics/graphics.h>

#include <y/utils/hash.h>

namespace yave {

// image layout inside the render pass (like color attachment optimal)
static VkImageLayout vk_initial_image_layout(ImageUsage usage) {
    return vk_image_layout(usage & ImageUsage::Attachment);
}

// image layout outside the renderpass (like shader read optimal)
static VkImageLayout vk_final_image_layout(ImageUsage usage) {
    return vk_image_layout(usage);
}

static std::array<VkSubpassDependency, 2> create_dependencies(bool depth, bool color) {
    VkAccessFlags access_flags = 0;
    VkPipelineStageFlags stages = 0;

    if(depth) {
        stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        access_flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    if(color) {
        stages |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        access_flags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    VkSubpassDependency top = {};
    {
        top.srcSubpass = VK_SUBPASS_EXTERNAL;
        top.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        top.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        top.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        top.dstStageMask = stages;
        top.dstAccessMask = access_flags;
    }

    VkSubpassDependency bot = {};
    {
        bot.dstSubpass = VK_SUBPASS_EXTERNAL;
        bot.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        bot.srcStageMask = stages;
        bot.srcAccessMask = access_flags;
        Y_TODO(Why do we need all commands with shader r/w?)
        bot.dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        bot.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    }

    return {top, bot};
}

static VkAttachmentLoadOp attachment_load_op(const RenderPass::AttachmentData& image) {
    return image.load_op == RenderPass::LoadOp::Clear
        ? VK_ATTACHMENT_LOAD_OP_CLEAR
        : VK_ATTACHMENT_LOAD_OP_LOAD;
}

static VkAttachmentDescription create_attachment(RenderPass::AttachmentData image) {
    VkAttachmentDescription description = {};
    {
        const auto layout = vk_final_image_layout(image.usage);

        description.format = image.format.vk_format();
        description.samples = VK_SAMPLE_COUNT_1_BIT;
        description.loadOp = attachment_load_op(image);
        description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        description.finalLayout = layout;
        description.initialLayout = layout;
    }
    return description;
}

static VkAttachmentReference create_attachment_reference(ImageUsage usage, usize index) {
    return VkAttachmentReference{u32(index), vk_initial_image_layout(usage)};
}

static VkRenderPass create_renderpass(RenderPass::AttachmentData depth, core::Span<RenderPass::AttachmentData> colors) {
    const bool has_depth = depth.usage != ImageUsage::None;
    const bool has_color = !colors.is_empty();

    auto attachments = core::vector_with_capacity<VkAttachmentDescription>(colors.size() + 1);
    std::transform(colors.begin(), colors.end(), std::back_inserter(attachments), [=](const auto& color) { return create_attachment(color); });

    auto color_refs = core::vector_with_capacity<VkAttachmentReference>(colors.size());
    for(usize i = 0; i != colors.size(); ++i) {
        color_refs << create_attachment_reference(ImageUsage::ColorBit, i);
    }

    VkSubpassDescription subpass = {};
    {
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = u32(color_refs.size());
        subpass.pColorAttachments = color_refs.data();
    }

    VkAttachmentReference depth_ref = {};
    if(has_depth) {
        attachments << create_attachment(depth);
        depth_ref = create_attachment_reference(ImageUsage::DepthBit, color_refs.size());
        subpass.pDepthStencilAttachment = &depth_ref;
    }

    const auto dependencies = create_dependencies(has_depth, has_color);

    VkRenderPassCreateInfo create_info = vk_struct();
    {
        create_info.attachmentCount = u32(attachments.size());
        create_info.pAttachments = attachments.data();
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass;
        create_info.dependencyCount = u32(dependencies.size());
        create_info.pDependencies = dependencies.data();
    }

    VkRenderPass renderpass = {};
    vk_check(vkCreateRenderPass(vk_device(), &create_info, vk_allocation_callbacks(), &renderpass));
    return renderpass;
}

RenderPass::Layout::Layout(AttachmentData depth, core::Span<AttachmentData> colors) : _depth(depth.format) {
    _colors.reserve(colors.size());
    std::transform(colors.begin(), colors.end(), std::back_inserter(_colors), [](const auto& e) { return e.format; });
}

u64 RenderPass::Layout::hash() const {
    u64 h = u64(_depth.vk_format());
    for(const auto& c : _colors) {
        hash_combine(h, u64(c.vk_format()));
    }
    return h;
}

bool RenderPass::Layout::is_depth_only() const {
    return _colors.is_empty();
}

bool RenderPass::Layout::operator==(const Layout& other) const {
    return _depth == other._depth && _colors == other._colors;
}


RenderPass::RenderPass(AttachmentData depth, core::Span<AttachmentData> colors) :
        _attachment_count(colors.size()),
        _render_pass(create_renderpass(depth, colors)),
        _layout(depth, colors) {
}

RenderPass::RenderPass(core::Span<AttachmentData> colors) :
        RenderPass(AttachmentData(), colors) {
}

RenderPass::~RenderPass() {
    device_destroy(_render_pass);
}

bool RenderPass::is_null() const {
    return !_render_pass;
}

VkRenderPass RenderPass::vk_render_pass() const {
    y_debug_assert(!is_null());
    return _render_pass;
}

bool RenderPass::is_depth_only() const {
    return layout().is_depth_only();
}

const RenderPass::Layout& RenderPass::layout() const {
    return _layout;
}

usize RenderPass::attachment_count() const {
    return _attachment_count;
}

}

