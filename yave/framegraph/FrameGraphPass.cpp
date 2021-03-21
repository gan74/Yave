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

#include "FrameGraphPass.h"
#include "FrameGraphFrameResources.h"
#include "FrameGraph.h"

#include <yave/graphics/device/extensions/DebugUtils.h>

namespace yave {

FrameGraphPass::FrameGraphPass(std::string_view name, FrameGraph* parent, usize index) : _name(name), _parent(parent), _index(index) {
}

const core::String& FrameGraphPass::name() const {
    return _name;
}

const FrameGraphFrameResources& FrameGraphPass::resources() const {
    return _parent->resources();
}

const Framebuffer& FrameGraphPass::framebuffer() const {
    if(_framebuffer.is_null()) {
        y_fatal("Pass has no framebuffer.");
    }
    return _framebuffer;
}

core::Span<DescriptorSet> FrameGraphPass::descriptor_sets() const {
    return _descriptor_sets;
}

void FrameGraphPass::render(CmdBufferRecorder& recorder) && {
    _render(recorder, this);
}

void FrameGraphPass::init_framebuffer(const FrameGraphFrameResources& resources) {
    if(_depth.image.is_valid() || _colors.size()) {
        auto declared_here = [&](FrameGraphImageId id) {
            const auto& info = _parent->info(id);
            return info.first_use == _index && !info.is_aliased();
        };

        Framebuffer::DepthAttachment depth;
        if(_depth.image.is_valid()) {
            depth = Framebuffer::DepthAttachment(resources.image<ImageUsage::DepthBit>(_depth.image), declared_here(_depth.image) ? Framebuffer::LoadOp::Clear : Framebuffer::LoadOp::Load);

#ifdef Y_DEBUG
            const auto& img = resources.image_base(_depth.image);
            if(const auto* debug = debug_utils()) {
                const core::String name = _name + " depth";
                debug->set_resource_name(img.vk_image(), name.data());
                debug->set_resource_name(img.vk_view(), name.data());
            }
#endif
        }
        auto colors = core::vector_with_capacity<Framebuffer::ColorAttachment>(_colors.size());
        for(auto&& color : _colors) {
            colors << Framebuffer::ColorAttachment(resources.image<ImageUsage::ColorBit>(color.image), declared_here(color.image) ? Framebuffer::LoadOp::Clear : Framebuffer::LoadOp::Load);

#ifdef Y_DEBUG
            const auto& img = resources.image_base(color.image);
            if(const auto* debug = debug_utils()) {
                const core::String name = (_name + " RT") + colors.size();
                debug->set_resource_name(img.vk_image(), name.data());
                debug->set_resource_name(img.vk_view(), name.data());
            }
#endif
        }
        _framebuffer = Framebuffer(depth, colors);

#ifdef Y_DEBUG
        if(const auto* debug = debug_utils()) {
            debug->set_resource_name(_framebuffer.vk_framebuffer(), (_name + " Framebuffer").data());
            debug->set_resource_name(_framebuffer.render_pass().vk_render_pass(), (_name + " RenderPass").data());
        }
#endif
    }
}

void FrameGraphPass::init_descriptor_sets(const FrameGraphFrameResources& resources) {
    for(const auto& set : _bindings) {
        auto bindings = core::vector_with_capacity<Descriptor>(set.size());
        std::transform(set.begin(), set.end(), std::back_inserter(bindings), [&](const FrameGraphDescriptorBinding& d) { return d.create_descriptor(resources); });
        _descriptor_sets << DescriptorSet(bindings);

#ifdef Y_DEBUG
        if(const auto* debug = debug_utils()) {
            const core::String name = (_name + " DS") + (_descriptor_sets.size() - 1);
            debug->set_resource_name(_descriptor_sets.last().vk_descriptor_set(), name.data());
        }
#endif
    }
}

}

