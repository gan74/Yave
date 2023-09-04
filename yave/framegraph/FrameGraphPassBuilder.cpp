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

#include "FrameGraphPassBuilder.h"
#include "FrameGraphPass.h"
#include "FrameGraph.h"

#include <y/utils/format.h>

namespace yave {

[[maybe_unused]]
static bool is_sampler_compatible(FrameGraph* framegraph, FrameGraphImageId img, SamplerType sampler) {
    y_debug_assert(framegraph);
    y_debug_assert(img.is_valid());
    return !is_linear(sampler) || framegraph->image_format(img).supports_filtering();
}

[[maybe_unused]]
static bool is_sampler_compatible(FrameGraph* framegraph, FrameGraphVolumeId img, SamplerType sampler) {
    y_debug_assert(framegraph);
    y_debug_assert(img.is_valid());
    return !is_linear(sampler) || framegraph->volume_format(img).supports_filtering();
}

FrameGraphPassBuilderBase::FrameGraphPassBuilderBase(FrameGraphPass* pass, PipelineStage default_stage) : _pass(pass), _default_stage(default_stage) {
}

void FrameGraphPassBuilderBase::set_render_func(render_func&& func) {
    y_debug_assert(_pass->_compute_render == nullptr && _pass->_render == nullptr);
    _pass->_render = std::move(func);
}

void FrameGraphPassBuilderBase::set_compute_render_func(compute_render_func &&func) {
    y_debug_assert(_pass->_compute_render == nullptr && _pass->_render == nullptr);
    _pass->_compute_render = std::move(func);
}

PipelineStage FrameGraphPassBuilderBase::or_default(PipelineStage stage) const {
    return stage == PipelineStage::None ? _default_stage : stage;
}


// --------------------------------- Declarations ---------------------------------

FrameGraphMutableImageId FrameGraphPassBuilderBase::declare_image(ImageFormat format, const math::Vec2ui& size) {
    return parent()->declare_image(format, size);
}

FrameGraphMutableVolumeId FrameGraphPassBuilderBase::declare_volume(ImageFormat format, const math::Vec3ui& size) {
    return parent()->declare_volume(format, size);
}

FrameGraphMutableBufferId FrameGraphPassBuilderBase::declare_buffer(u64 byte_size) {
    return parent()->declare_buffer(byte_size);
}

FrameGraphMutableImageId FrameGraphPassBuilderBase::declare_copy(FrameGraphImageId src) {
    const auto src_info = parent()->info(src); // Not a ref: declare_image invalidates ImageCreateInfo references
    const auto res = declare_image(src_info.format, src_info.size.to<2>());

    // copies are done before the pass proper so we don't set the stage
    add_to_pass(src, ImageUsage::TransferSrcBit, false, PipelineStage::None);
    add_to_pass(res, ImageUsage::TransferDstBit, true, PipelineStage::None);

    parent()->register_image_copy(res, src, _pass);

    return res;
}

FrameGraphMutableBufferId FrameGraphPassBuilderBase::declare_copy(FrameGraphBufferId src) {
    const auto src_info = parent()->info(src);
    const auto res = declare_buffer(src_info.byte_size);

    // copies are done before the pass proper so we don't set the stage
    add_to_pass(src, BufferUsage::TransferSrcBit, false, PipelineStage::None);
    add_to_pass(res, BufferUsage::TransferDstBit, true, PipelineStage::None);

    parent()->register_buffer_copy(res, src, _pass);

    return res;
}


// --------------------------------- Usage ---------------------------------

void FrameGraphPassBuilderBase::add_input_usage(FrameGraphImageId res, ImageUsage usage) {
    add_to_pass(res, usage, false, PipelineStage::None);
}

void FrameGraphPassBuilderBase::add_input_usage(FrameGraphBufferId res, BufferUsage usage) {
    add_to_pass(res, usage, false, PipelineStage::None);
}

void FrameGraphPassBuilderBase::add_output_usage(FrameGraphMutableImageId res, ImageUsage usage) {
    add_to_pass(res, usage, true, PipelineStage::None);
}

void FrameGraphPassBuilderBase::add_output_usage(FrameGraphBufferId res, BufferUsage usage) {
    add_to_pass(res, usage, true, PipelineStage::None);
}


// --------------------------------- Framebuffer ---------------------------------

void FrameGraphPassBuilderBase::add_depth_output(FrameGraphMutableImageId res) {
    // transition is done by the renderpass
    add_to_pass(res, ImageUsage::DepthBit, true, PipelineStage::DepthAttachmentOutBit);
    if(_pass->_depth.image.is_valid()) {
        y_fatal("Pass already has a depth output.");
    }
    _pass->_depth = FrameGraphPass::Attachment{res};
}

void FrameGraphPassBuilderBase::add_color_output(FrameGraphMutableImageId res) {
    // transition is done by the renderpass
    add_to_pass(res, ImageUsage::ColorBit, true, PipelineStage::ColorAttachmentOutBit);
    _pass->_colors << FrameGraphPass::Attachment{res};
}


// --------------------------------- Storage output ---------------------------------

void FrameGraphPassBuilderBase::add_storage_output(FrameGraphMutableImageId res, PipelineStage stage, i32 ds_index) {
    add_to_pass(res, ImageUsage::StorageBit, true, or_default(stage));
    add_descriptor_binding(FrameGraphDescriptorBinding::create_storage_binding(res), ds_index);
}

void FrameGraphPassBuilderBase::add_storage_output(FrameGraphMutableVolumeId res, PipelineStage stage, i32 ds_index) {
    add_to_pass(res, ImageUsage::StorageBit, true, or_default(stage));
    add_descriptor_binding(FrameGraphDescriptorBinding::create_storage_binding(res), ds_index);
}

void FrameGraphPassBuilderBase::add_storage_output(FrameGraphMutableBufferId res, PipelineStage stage, i32 ds_index) {
    add_to_pass(res, BufferUsage::StorageBit, true, or_default(stage));
    add_descriptor_binding(FrameGraphDescriptorBinding::create_storage_binding(res), ds_index);
}


// --------------------------------- Storage intput ---------------------------------

void FrameGraphPassBuilderBase::add_storage_input(FrameGraphBufferId res, PipelineStage stage, i32 ds_index) {
    add_to_pass(res, BufferUsage::StorageBit, false, or_default(stage));
    add_descriptor_binding(FrameGraphDescriptorBinding::create_storage_binding(res), ds_index);
}

void FrameGraphPassBuilderBase::add_storage_input(FrameGraphVolumeId res, PipelineStage stage, i32 ds_index) {
    add_to_pass(res, ImageUsage::StorageBit, false, or_default(stage));
    add_descriptor_binding(FrameGraphDescriptorBinding::create_storage_binding(res), ds_index);
}

void FrameGraphPassBuilderBase::add_storage_input(FrameGraphImageId res, PipelineStage stage, i32 ds_index) {
    add_to_pass(res, ImageUsage::StorageBit, false, or_default(stage));
    add_descriptor_binding(FrameGraphDescriptorBinding::create_storage_binding(res), ds_index);
}


// --------------------------------- Uniform input ---------------------------------

void FrameGraphPassBuilderBase::add_uniform_input(FrameGraphBufferId res, PipelineStage stage, i32 ds_index) {
    add_to_pass(res, BufferUsage::UniformBit, false, or_default(stage));
    add_descriptor_binding(FrameGraphDescriptorBinding::create_uniform_binding(res), ds_index);
}

void FrameGraphPassBuilderBase::add_uniform_input(FrameGraphVolumeId res, PipelineStage stage, i32 ds_index) {
    Y_TODO(optimize?)
    const bool filter = parent()->volume_format(res).supports_filtering();
    add_uniform_input(res, default_samplers[filter], stage, ds_index);
}

void FrameGraphPassBuilderBase::add_uniform_input(FrameGraphImageId res, PipelineStage stage, i32 ds_index) {
    Y_TODO(optimize?)
    const bool filter = parent()->image_format(res).supports_filtering();
    add_uniform_input(res, default_samplers[filter], stage, ds_index);
}

void FrameGraphPassBuilderBase::add_uniform_input(FrameGraphVolumeId res, SamplerType sampler, PipelineStage stage, i32 ds_index) {
    y_debug_assert(is_sampler_compatible(parent(), res, sampler));

    add_to_pass(res, ImageUsage::TextureBit, false, or_default(stage));
    add_descriptor_binding(FrameGraphDescriptorBinding::create_uniform_binding(res, sampler), ds_index);
}

void FrameGraphPassBuilderBase::add_uniform_input(FrameGraphImageId res, SamplerType sampler, PipelineStage stage, i32 ds_index) {
    y_debug_assert(is_sampler_compatible(parent(), res, sampler));

    add_to_pass(res, ImageUsage::TextureBit, false, or_default(stage));
    add_descriptor_binding(FrameGraphDescriptorBinding::create_uniform_binding(res, sampler), ds_index);
}

void FrameGraphPassBuilderBase::add_uniform_input_with_default(FrameGraphImageId res, Descriptor desc, PipelineStage stage, i32 ds_index) {
    y_debug_assert(!desc.is_inline_block());
    if(res.is_valid()) {
        add_uniform_input(res, stage, ds_index);
    } else {
        add_external_input(desc, stage, ds_index);
    }
}


// --------------------------------- Inline ---------------------------------

void FrameGraphPassBuilderBase::add_inline_input(InlineDescriptor desc, i32 ds_index) {
    add_descriptor_binding(Descriptor(parent()->copy_inline_descriptor(desc)), ds_index);
}


// --------------------------------- External ---------------------------------

Y_TODO(external framegraph resources are not synchronized)
void FrameGraphPassBuilderBase::add_external_input(Descriptor desc, PipelineStage, i32 ds_index) {
    y_debug_assert(!desc.is_inline_block());
    add_descriptor_binding(desc, ds_index);
}


// --------------------------------- Attribs ---------------------------------

void FrameGraphPassBuilderBase::add_attrib_input(FrameGraphBufferId res, PipelineStage stage) {
    add_to_pass(res, BufferUsage::AttributeBit, false, stage);
}

void FrameGraphPassBuilderBase::add_index_input(FrameGraphBufferId res, PipelineStage stage) {
    add_to_pass(res, BufferUsage::IndexBit, false, stage);
}


// --------------------------------- Stuff ---------------------------------

void FrameGraphPassBuilderBase::clear_before_pass(FrameGraphMutableImageId res) {
    add_to_pass(res, ImageUsage::TransferDstBit, true, PipelineStage::None);
    parent()->register_image_clear(res, _pass);
}

void FrameGraphPassBuilderBase::add_descriptor_binding(Descriptor desc, i32 ds_index) {
    add_descriptor_binding(FrameGraphDescriptorBinding(desc), ds_index);
}

i32 FrameGraphPassBuilderBase::next_descriptor_set_index() const {
    const auto& bindings = _pass->_bindings;
    return i32(bindings.size());
}

template<typename T>
void set_stage(const FrameGraphPass* pass, T& info, PipelineStage stage) {
    if(info.stage != PipelineStage::None) {
        Y_TODO(This should be either one write or many reads)
        y_fatal("Resource can only be used once per pass (used twice by \"{}\", previous stage was {}).", pass->name(), usize(info.stage));
    }
    info.stage = stage;
}

void FrameGraphPassBuilderBase::add_to_pass(FrameGraphImageId res, ImageUsage usage, bool is_written, PipelineStage stage) {
    res.check_valid();
    auto& info = _pass->info(res);
    info.written_to |= is_written;
    set_stage(_pass, info, stage);
    parent()->register_usage(res, usage, is_written, _pass);
}

void FrameGraphPassBuilderBase::add_to_pass(FrameGraphVolumeId res, ImageUsage usage, bool is_written, PipelineStage stage) {
    res.check_valid();
    auto& info = _pass->info(res);
    info.written_to |= is_written;
    set_stage(_pass, info, stage);
    parent()->register_usage(res, usage, is_written, _pass);
}

void FrameGraphPassBuilderBase::add_to_pass(FrameGraphBufferId res, BufferUsage usage, bool is_written, PipelineStage stage) {
    res.check_valid();
    auto& info = _pass->info(res);
    info.written_to |= is_written;
    set_stage(_pass, info, stage);
    parent()->register_usage(res, usage, is_written, _pass);
}

void FrameGraphPassBuilderBase::add_descriptor_binding(FrameGraphDescriptorBinding binding, i32 ds_index) {
    auto& bindings = _pass->_bindings;
    if(ds_index < 0) {
        bindings.set_min_size(-ds_index);
        bindings[bindings.size() + ds_index].push_back(binding);
    } else {
        bindings.set_min_size(ds_index + 1);
        bindings[ds_index].push_back(binding);
    }
}

void FrameGraphPassBuilderBase::map_buffer_internal(FrameGraphMutableBufferId res, InlineDescriptor desc) {
    parent()->map_buffer(res, _pass);
    if(desc.data()) {
        _pass->_map_data.emplace_back(res, parent()->copy_inline_descriptor(desc));
    }
}

FrameGraph* FrameGraphPassBuilderBase::parent() const {
    return _pass->_parent;
}

}

