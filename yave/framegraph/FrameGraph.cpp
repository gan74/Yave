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

#include "FrameGraph.h"
#include "FrameGraphPass.h"
#include "FrameGraphFrameResources.h"

#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/utils/color.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

static void check_usage_io(ImageUsage usage, bool is_output) {
    unused(usage, is_output);
    switch(usage) {
        case ImageUsage::TextureBit:
        case ImageUsage::TransferSrcBit:
            y_debug_assert(!is_output);
        break;

        case ImageUsage::DepthBit:
        case ImageUsage::ColorBit:
        case ImageUsage::TransferDstBit:
            y_debug_assert(is_output);
        break;

        default:
        break;
    }
}

static void check_usage_io(BufferUsage usage, bool is_output) {
    unused(usage, is_output);
    switch(usage) {
        case BufferUsage::AttributeBit:
        case BufferUsage::IndexBit:
        case BufferUsage::IndirectBit:
        case BufferUsage::UniformBit:
        case BufferUsage::TransferSrcBit:
            y_debug_assert(!is_output);
        break;

        case BufferUsage::TransferDstBit:
            y_debug_assert(is_output);
        break;

        default:
        break;
    }
}

template<typename U>
static bool is_none(U u) {
    return u == U::None;
}

template<typename T, typename C>
static auto&& check_exists(C& c, T t) {
    const auto it = c.find(t);
    if(it == c.end()) {
        y_fatal("Resource doesn't exist");
    }
    return it->second;
}

template<typename C, typename B>
static void build_barriers(const C& resources, B& barriers, std::unordered_map<FrameGraphResourceId, PipelineStage>& to_barrier, FrameGraphFrameResources& frame_res) {
    for(auto&& [res, info] : resources) {
        const auto it = to_barrier.find(res);
        const bool exists = it != to_barrier.end();
        // barrier around attachments are handled by the renderpass
        const PipelineStage stage = info.stage & ~PipelineStage::AllAttachmentOutBit;
        if(stage != PipelineStage::None) {
            if(exists) {
                barriers.emplace_back(frame_res.barrier(res, it->second, info.stage));
                it->second = info.stage;
            } else {
                to_barrier[res] = info.stage;
            }
        }

        if(exists) {
            to_barrier.erase(it);
        }
    }
}

static void copy_image(CmdBufferRecorder& recorder, FrameGraphImageId src, FrameGraphMutableImageId dst,
                        std::unordered_map<FrameGraphResourceId, PipelineStage>& to_barrier, const FrameGraphFrameResources& resources) {

    Y_TODO(We might end up barriering twice here)
    if(resources.are_aliased(src, dst)) {
        if(const auto it = to_barrier.find(src); it != to_barrier.end()) {
            to_barrier[dst] = it->second;
            to_barrier.erase(it);
        }
    } else {
        to_barrier.erase(src);
        to_barrier.erase(dst);
        recorder.barriered_copy(resources.image_base(src), resources.image_base(dst));
    }
}

[[maybe_unused]]
static void copy_images(CmdBufferRecorder& recorder, core::Span<std::pair<FrameGraphImageId, FrameGraphMutableImageId>> copies,
                        std::unordered_map<FrameGraphResourceId, PipelineStage>& to_barrier, const FrameGraphFrameResources& resources) {

    for(auto [src, dst] : copies) {
        copy_image(recorder, src, dst, to_barrier, resources);
    }
}



FrameGraphRegion::~FrameGraphRegion() {
    y_debug_assert(_parent);
    _parent->end_region(_index);
}

FrameGraphRegion::FrameGraphRegion(FrameGraph* parent, usize index) : _parent(parent), _index(index) {
}

FrameGraph::FrameGraph(std::shared_ptr<FrameGraphResourcePool> pool) : _resources(std::make_unique<FrameGraphFrameResources>(std::move(pool))) {
}

FrameGraph::~FrameGraph() {
}

const FrameGraphFrameResources& FrameGraph::resources() const {
    return *_resources;
}

FrameGraphRegion FrameGraph::region(std::string_view name) {
    const usize index = _regions.size();
    _regions.emplace_back(Region{name, _pass_index + 1, _pass_index + 1});
    return FrameGraphRegion(this, index);
}

void FrameGraph::end_region(usize index) {
    _regions[index].end_pass = _pass_index;
}




void FrameGraph::render(CmdBufferRecorder& recorder) && {
    y_profile();
    Y_TODO(Pass culling)
    Y_TODO(Ensure that pass are always recorded in order)

    // -------------------- region stuff --------------------
    const auto frame_region = recorder.region("Framegraph render", math::Vec4(0.7f, 0.7f, 0.7f, 1.0f));

    struct RuntimeRegion {
        Region region;
        CmdBufferRegion cmd;
        math::Vec4 color;

        math::Vec4 next_color() {
            color = color * 0.8f + 0.2f;
            return color;
        }
    };

    usize next_region_index = 0;
    core::Vector<RuntimeRegion> regions;
    auto next_color = [id = 0]() mutable {
        return math::Vec4(identifying_color(id++), 1.0f);
    };

    auto begin_pass_region = [&](const FrameGraphPass& pass) {
        while(true) {
            if(next_region_index < _regions.size() && _regions[next_region_index].begin_pass == pass._index) {
                const math::Vec4 color = next_color();
                regions.emplace_back(RuntimeRegion{_regions[next_region_index], recorder.region(_regions[next_region_index].name.data(), color), color});
                ++next_region_index;
            } else {
                break;
            }
        }

        const math::Vec4 color = regions.is_empty() ? next_color() : regions.last().next_color();
        return recorder.region(pass.name().data(), color);
    };

    auto end_pass_region = [&](const FrameGraphPass& pass) {
        for(usize i = 0; i != regions.size(); ++i) {
            if(regions[i].region.end_pass <= pass._index) {
                regions.erase_unordered(regions.begin() + i);
                --i;
            }
        }
    };



    // -------------------- resource management --------------------
    alloc_resources();

    usize copy_index = 0;
    std::sort(_image_copies.begin(), _image_copies.end(), [&](const auto& a, const auto& b) { return a.pass_index < b.pass_index; });

    std::unordered_map<FrameGraphResourceId, PipelineStage> to_barrier;
    core::Vector<BufferBarrier> buffer_barriers;
    core::Vector<ImageBarrier> image_barriers;


    {
        y_profile_zone("init");
        for(const auto& pass : _passes) {
            y_profile_dyn_zone(pass->name().data());
            pass->init_framebuffer(*_resources);
            pass->init_descriptor_sets(*_resources);
        }
    }

    {
        y_profile_zone("render");
        for(const auto& pass : _passes) {
            y_profile_dyn_zone(pass->name().data());
            const auto region = begin_pass_region(*pass);

            {
                y_profile_zone("prepare");
                while(copy_index < _image_copies.size() && _image_copies[copy_index].pass_index == pass->_index) {
                    // copie_image will not do anything if the two are aliased
                    copy_image(recorder, _image_copies[copy_index].src, _image_copies[copy_index].dst, to_barrier, *_resources);
                    ++copy_index;
                }
            }

            {
                y_profile_zone("barriers");
                buffer_barriers.make_empty();
                image_barriers.make_empty();
                build_barriers(pass->_buffers, buffer_barriers, to_barrier, *_resources);
                build_barriers(pass->_images, image_barriers, to_barrier, *_resources);
                recorder.barriers(buffer_barriers, image_barriers);

                //recorder.full_barrier();
            }

            {
                y_profile_zone("render");
                std::move(*pass).render(recorder);
            }

            end_pass_region(*pass);
        }
    }

    Y_TODO(Only keep alive cpu mapped buffers)
    recorder.keep_alive(std::move(_resources));

    Y_TODO(Put resource barriers at the end of the graph to prevent clash with whatever comes after)
}

void FrameGraph::alloc_resources() {
    y_profile();

    for(const auto& cpy : _image_copies) {
        y_debug_assert(cpy.pass_index <= check_exists(_images, cpy.dst).first_use);

        auto& dst_info = check_exists(_images, cpy.dst);
        auto* src_info = &check_exists(_images, cpy.src);

        while(src_info->alias.is_valid()) {
            src_info = &check_exists(_images, src_info->alias);
        }

        const usize src_last_use = src_info->last_use();
        if(src_last_use < dst_info.first_use || (src_last_use == dst_info.first_use && src_info->can_alias_on_last)) {
            src_info->register_alias(dst_info);

            dst_info.alias = dst_info.copy_src;
            dst_info.copy_src = FrameGraphImageId();
        }
    }

    auto images = core::vector_with_capacity<std::pair<FrameGraphImageId, ImageCreateInfo>>(_images.size());
    std::copy(_images.begin(), _images.end(), std::back_inserter(images));
    std::sort(images.begin(), images.end(), [](const auto& a, const auto& b) { return a.second.first_use < b.second.first_use; });

    _resources->reserve(_images.size(), _buffers.size());

    for(auto&& [res, info] : images) {
        /*if(info.last_read < info.last_write && (info.usage & ImageUsage::Attachment) == ImageUsage::None) {
            log_msg(fmt("Image written by % is never consumed", pass_name(info.last_write)), Log::Warning);
        }*/
        if(info.alias.is_valid()) {
            y_debug_assert(allow_image_aliasing);
            _resources->create_alias(res, info.alias);
        } else {
            if(!info.has_usage()) {
                log_msg(fmt("Image declared by % has no usage", pass_name(info.first_use)), Log::Warning);
                // All images should support texturing, hopefully
                info.usage = info.usage | ImageUsage::TextureBit;
            }
            _resources->create_image(res, info.format, info.size, info.usage);
        }
    }

    for(auto&& [res, info] : _buffers) {
        if(info.last_read < info.last_write) {
            log_msg(fmt("Buffer written by % is never consumed", pass_name(info.last_write)), Log::Warning);
        }
        if(is_none(info.usage)) {
            log_msg("Unused frame graph buffer resource", Log::Warning);
            info.usage = info.usage | BufferUsage::StorageBit;
        }
        _resources->create_buffer(res, info.byte_size, info.usage, info.memory_type);
    }
}

const core::String& FrameGraph::pass_name(usize pass_index) const {
    for(const auto& pass : _passes) {
        if(pass->_index == pass_index) {
            return pass->name();
        }
    }
    y_fatal("Pass index out of bounds (%)", pass_index);
}

FrameGraphMutableImageId FrameGraph::declare_image(ImageFormat format, const math::Vec2ui& size) {
    y_debug_assert(size.x() > 0 && size.y() > 0);
    FrameGraphMutableImageId res;
    res._id = _resources->create_resource_id();
    auto& r = _images[res];
    r.size = size;
    r.format = format;
    return res;
}

FrameGraphMutableBufferId FrameGraph::declare_buffer(usize byte_size) {
    y_debug_assert(byte_size > 0);
    FrameGraphMutableBufferId res;
    res._id = _resources->create_resource_id();
    auto& r = _buffers[res];
    r.byte_size = byte_size;
    return res;
}

FrameGraphPassBuilder FrameGraph::add_pass(std::string_view name) {
    auto pass = std::make_unique<FrameGraphPass>(name, this, ++_pass_index);
    FrameGraphPass* ptr = pass.get();
     _passes << std::move(pass);
    return FrameGraphPassBuilder(ptr);
}

const FrameGraph::ImageCreateInfo& FrameGraph::info(FrameGraphImageId res) const {
    return check_exists(_images, res);
}

const FrameGraph::BufferCreateInfo& FrameGraph::info(FrameGraphBufferId res) const {
    return check_exists(_buffers, res);
}



usize FrameGraph::ResourceCreateInfo::last_use() const {
    return std::max(last_read, last_write);
}

void FrameGraph::ResourceCreateInfo::register_use(usize index, bool is_written) {
    usize& last = is_written ? last_write : last_read;
    last = std::max(last, index);
    if(!first_use) {
        first_use = index;
    }
}

void FrameGraph::ImageCreateInfo::register_alias(const ImageCreateInfo& other) {
    y_debug_assert(other.size == size);
    y_debug_assert(other.format == format);
    y_debug_assert(other.first_use > last_write);

    last_write = std::max(last_write, other.last_write);
    last_read = std::max(last_read, other.last_read);
    usage = usage | other.usage;
    can_alias_on_last = false;
}

bool FrameGraph::ImageCreateInfo::is_aliased() const {
    return copy_src.is_valid() || alias.is_valid();
}

bool FrameGraph::ImageCreateInfo::has_usage() const {
    return (usage & ~ImageUsage::TransferDstBit) != ImageUsage::None;
}

void FrameGraph::register_usage(FrameGraphImageId res, ImageUsage usage, bool is_written, const FrameGraphPass* pass) {
    check_usage_io(usage, is_written);
    auto& info = check_exists(_images, res);
    info.usage = info.usage | usage;

    const bool can_alias = allow_image_aliasing && (info.last_use() != pass->_index || info.can_alias_on_last);
    info.register_use(pass->_index, is_written);

    // copies are done before the pass so we can alias even if the image is copied
    if(can_alias && usage == ImageUsage::TransferSrcBit) {
        info.can_alias_on_last = true;
    } else if(usage != ImageUsage::TransferSrcBit) {
        info.can_alias_on_last = false;
    }
}

void FrameGraph::register_usage(FrameGraphBufferId res, BufferUsage usage, bool is_written, const FrameGraphPass* pass) {
    check_usage_io(usage, is_written);
    auto& info = check_exists(_buffers, res);
    info.usage = info.usage | usage;
    info.register_use(pass->_index, is_written);
}

void FrameGraph::register_image_copy(FrameGraphMutableImageId dst, FrameGraphImageId src, const FrameGraphPass* pass) {
    auto& info = check_exists(_images, dst);
    if(info.copy_src.is_valid()) {
        y_fatal("Resource is already a copy");
    }
    info.copy_src = src;
    _image_copies.push_back({pass->_index, dst, src});
}

InlineDescriptor FrameGraph::copy_inline_descriptor(InlineDescriptor desc) {
    const usize desc_word_size = desc.size_in_words();

    for(InlineStorage& storage : _inline_storage) {
        if(storage.storage.size() - storage.used < desc_word_size) {
            continue;
        }

        u32* begin = storage.storage.data() + storage.used;
        const InlineDescriptor ret(core::Span<u32>(begin, desc_word_size));
        std::copy_n(desc.words(), desc_word_size, begin);
        storage.used += desc_word_size;
        return ret;
    }

    // 64KB blocks
    const usize size = std::max(usize(16 * 1024), desc_word_size);
    InlineStorage& storage = _inline_storage.emplace_back(size);

    u32* begin = storage.storage.data() + storage.used;
    const InlineDescriptor ret(core::Span<u32>(begin, desc_word_size));
    std::copy_n(desc.words(), desc_word_size, begin);
    storage.used += desc_word_size;
    return ret;
}

void FrameGraph::set_cpu_visible(FrameGraphMutableBufferId res, const FrameGraphPass* pass) {
    auto& info = check_exists(_buffers, res);
    info.memory_type = MemoryType::CpuVisible;
    info.register_use(pass->_index, true);
}

bool FrameGraph::is_attachment(FrameGraphImageId res) const {
    const auto& info = check_exists(_images, res);
    return (info.usage & ImageUsage::Attachment) != ImageUsage::None;
}

math::Vec2ui FrameGraph::image_size(FrameGraphImageId res) const {
    const auto& info = check_exists(_images, res);
    return info.size;
}

ImageFormat FrameGraph::image_format(FrameGraphImageId res) const {
    const auto& info = check_exists(_images, res);
    return info.format;
}

}

