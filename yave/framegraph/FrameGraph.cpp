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

#include "FrameGraph.h"
#include "FrameGraphPass.h"
#include "FrameGraphFrameResources.h"

#include <yave/graphics/commands/CmdQueue.h>

#include <yave/utils/color.h>

#include <y/core/ScratchPad.h>
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
    if(t.id() < c.size()) {
        auto&& [i, r] = c[t.id()];
        if(i.is_valid()) {
            return r;
        }
    }
    y_fatal("Resource doesn't exist");
}

template<typename C, typename B, typename H>
static void build_barriers(const C& resources, B& barriers, H& to_barrier, const FrameGraphFrameResources& frame_res) {
    for(auto&& [res, info] : resources) {
        // barrier around attachments are handled by the renderpass
        const PipelineStage stage = info.stage & ~PipelineStage::AllAttachmentOutBit;
        if(stage != PipelineStage::None) {
            const auto it = to_barrier.find(res);
            const bool exists = it != to_barrier.end();
            if(exists) {
                barriers.emplace_back(frame_res.barrier(res, it->second, info.stage));
                to_barrier.erase(it);
            }

            if(info.written_to) {
                to_barrier[res] = info.stage;
            }
        }
    }
}

template<typename H>
static void copy_image(CmdBufferRecorder& recorder, FrameGraphImageId src, FrameGraphMutableImageId dst,
                       H& to_barrier, const FrameGraphFrameResources& frame_res) {

    Y_TODO(We might end up barriering twice here)
    if(frame_res.are_aliased(src, dst)) {
        if(const auto it = to_barrier.find(src); it != to_barrier.end()) {
            to_barrier[dst] = it->second;
            to_barrier.erase(it);
        }
    } else {
        to_barrier.erase(src);
        to_barrier.erase(dst);
        recorder.copy(frame_res.image_base(src), frame_res.image_base(dst));
    }
}

template<typename H, typename B>
static void copy_buffer(CmdBufferRecorder& recorder, FrameGraphBufferId src, FrameGraphMutableBufferId dst,
                        H& to_barrier, const B& pass_buffers, const FrameGraphFrameResources& frame_res) {

    if(const auto src_it = to_barrier.find(src); src_it != to_barrier.end()) {
        const auto dst_it = pass_buffers.find(dst);
        y_debug_assert(dst_it != pass_buffers.end());
        recorder.barriers(frame_res.barrier(src, src_it->second, dst_it->second.stage));
        to_barrier.erase(src_it);
    }

    auto& dst_stage = to_barrier[dst];
    if(dst_stage != PipelineStage::None) {
        recorder.barriers(frame_res.barrier(dst, dst_stage, PipelineStage::TransferBit));
    }

    recorder.unbarriered_copy(frame_res.buffer<BufferUsage::TransferSrcBit>(src), frame_res.buffer<BufferUsage::TransferDstBit>(dst));

    dst_stage = PipelineStage::TransferBit;
}

template<typename H>
static void clear_image(CmdBufferRecorder& recorder, FrameGraphMutableImageId dst,
                        H& to_barrier, const FrameGraphFrameResources& frame_res) {

    to_barrier.erase(dst);
    recorder.clear(frame_res.image_base(dst));
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

u64 FrameGraph::frame_id() const {
    return _resources->frame_id();
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




void FrameGraph::render(CmdBufferRecorder& recorder, CmdTimingRecorder* time_rec) {
    y_profile();
    Y_TODO(Pass culling)
    Y_TODO(Ensure that pass are always recorded in order)

    // -------------------- region stuff --------------------
    const auto frame_region = recorder.region("Framegraph render", time_rec, math::Vec4(0.7f, 0.7f, 0.7f, 1.0f));

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
    core::ScratchVector<RuntimeRegion> regions(_passes.size());

    auto next_color = [id = 0]() mutable {
        return math::Vec4(identifying_color(id++), 1.0f);
    };

    auto begin_pass_region = [&](const FrameGraphPass& pass) {
        while(true) {
            if(next_region_index < _regions.size() && _regions[next_region_index].begin_pass == pass._index) {
                const math::Vec4 color = next_color();
                regions.emplace_back(RuntimeRegion{_regions[next_region_index], recorder.region(_regions[next_region_index].name.data(), time_rec, color), color});
                ++next_region_index;
            } else {
                break;
            }
        }

        const math::Vec4 color = regions.is_empty() ? next_color() : regions.last().next_color();
        return recorder.region(pass.name().data(), time_rec, color);
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

    usize image_copy_index = 0;
    std::sort(_image_copies.begin(), _image_copies.end(), [&](const auto& a, const auto& b) { return a.pass_index < b.pass_index; });

    usize buffer_copy_index = 0;
    std::sort(_buffer_copies.begin(), _buffer_copies.end(), [&](const auto& a, const auto& b) { return a.pass_index < b.pass_index; });

    usize image_clear_index = 0;
    std::sort(_image_clears.begin(), _image_clears.end(), [&](const auto& a, const auto& b) { return a.pass_index < b.pass_index; });

    using hash_t = std::hash<FrameGraphResourceId>;
    core::FlatHashMap<FrameGraphBufferId, PipelineStage, hash_t> buffers_to_barrier;
    core::FlatHashMap<FrameGraphVolumeId, PipelineStage, hash_t> volumes_to_barrier;
    core::FlatHashMap<FrameGraphImageId, PipelineStage, hash_t> images_to_barrier;
    buffers_to_barrier.set_min_capacity(_buffers.size());
    volumes_to_barrier.set_min_capacity(_volumes.size());
    images_to_barrier.set_min_capacity(_images.size());

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
                for(const auto& [res, data] : pass->_map_data) {
                    auto mapping = _resources->map_buffer_bytes(res);
                    std::memcpy(mapping.data(), data.data(), data.size());
                }

                while(image_copy_index < _image_copies.size() && _image_copies[image_copy_index].pass_index == pass->_index) {
                    // copie_image will not do anything if the two are aliased
                    copy_image(recorder, _image_copies[image_copy_index].src, _image_copies[image_copy_index].dst, images_to_barrier, *_resources);
                    ++image_copy_index;
                }

                while(buffer_copy_index < _buffer_copies.size() && _buffer_copies[buffer_copy_index].pass_index == pass->_index) {
                    copy_buffer(recorder, _buffer_copies[buffer_copy_index].src, _buffer_copies[buffer_copy_index].dst, buffers_to_barrier, pass->_buffers, *_resources);
                    ++buffer_copy_index;
                }

                while(image_clear_index < _image_clears.size() && _image_clears[image_clear_index].pass_index == pass->_index) {
                    clear_image(recorder, _image_clears[image_clear_index].dst, images_to_barrier, *_resources);
                    ++image_clear_index;
                }
            }

            {
                y_profile_zone("barriers");

                core::ScratchVector<BufferBarrier> buffer_barriers(pass->_buffers.size());
                core::ScratchVector<ImageBarrier> image_barriers(pass->_images.size() + pass->_volumes.size());
                build_barriers(pass->_buffers, buffer_barriers, buffers_to_barrier, *_resources);
                build_barriers(pass->_volumes, image_barriers, volumes_to_barrier, *_resources);
                build_barriers(pass->_images, image_barriers, images_to_barrier, *_resources);
                recorder.barriers(buffer_barriers, image_barriers);
            }

            {
                y_profile_zone("render");
                pass->render(recorder);
            }

            end_pass_region(*pass);

        }
    }

    {
        TransferCmdBufferRecorder prepare = create_disposable_transfer_cmd_buffer();
        _resources->flush_mapped_buffers(prepare);
        prepare.submit_async();
    }

    Y_TODO(Put resource barriers at the end of the graph to prevent clash with whatever comes after)
}

void FrameGraph::alloc_resources() {
    y_profile();

    if constexpr(allow_image_aliasing) {
        for(const auto& cpy : _image_copies) {
            y_debug_assert(cpy.pass_index <= check_exists(_images, cpy.dst).first_use);

            auto& dst_info = check_exists(_images, cpy.dst);
            auto* src_info = &check_exists(_images, cpy.src);

            y_debug_assert(!dst_info.is_prev());

            while(src_info->alias.is_valid()) {
                src_info = &check_exists(_images, src_info->alias);
            }

            if(src_info->is_persistent()) {
                continue;
            }

            const usize src_last_use = src_info->last_use();
            // copies are done before the pass so we can alias even if the image is copied
            const bool can_alias_on_last = src_info->last_usage == ImageUsage::TransferSrcBit;
            if(src_last_use < dst_info.first_use || (src_last_use == dst_info.first_use && can_alias_on_last)) {
                src_info->register_alias(dst_info);

                dst_info.alias = dst_info.copy_src;
                dst_info.copy_src = FrameGraphImageId();
            }
        }
    }

    core::ScratchVector<std::pair<FrameGraphImageId, ImageCreateInfo>> images(_images.size());
    std::copy_if(_images.begin(), _images.end(), std::back_inserter(images), [](const auto& p) { return p.first.is_valid(); });
    std::sort(images.begin(), images.end(), [](const auto& a, const auto& b) { return a.second.first_use < b.second.first_use; });

    for(auto&& [res, info] : images) {
        if(info.is_prev()) {
            y_debug_assert(_resources->is_alive(res));
            continue;
        }

        if(info.alias.is_valid()) {
            y_debug_assert(allow_image_aliasing);
            _resources->create_alias(res, info.alias);
        } else {
            if(!info.has_usage()) {
                log_msg(fmt("Image declared by {} has no usage", pass_name(info.first_use)), Log::Warning);
                // All images should support texturing, hopefully
                info.usage = info.usage | ImageUsage::TextureBit;
            }
            _resources->create_image(res, info.format, info.size.to<2>(), info.usage, info.persistent);
        }
    }

    for(auto&& [res, info] : _volumes) {
        if(info.is_prev()) {
            y_debug_assert(_resources->is_alive(res));
            continue;
        }

        y_always_assert(!info.alias.is_valid(), "Volume aliasing not supported");
        if(!info.has_usage()) {
            log_msg(fmt("Volume declared by {} has no usage", pass_name(info.first_use)), Log::Warning);
            // All images should support texturing, hopefully
            info.usage = info.usage | ImageUsage::TextureBit;
        }
        _resources->create_volume(res, info.format, info.size, info.usage, info.persistent);
    }

    for(auto&& [res, info] : _buffers) {
        if(info.is_prev()) {
            y_debug_assert(_resources->is_alive(res));
            continue;
        }

        if(info.last_read < info.last_write) {
            log_msg(fmt("Buffer written by {} is never consumed", pass_name(info.last_write)), Log::Warning);
        }
        if(is_none(info.usage)) {
            log_msg("Unused frame graph buffer resource", Log::Warning);
            info.usage = info.usage | BufferUsage::StorageBit;
        }
        _resources->create_buffer(res, info.byte_size, info.usage, info.memory_type, info.persistent);
    }

    _resources->init_staging_buffer();
}

const core::String& FrameGraph::pass_name(usize pass_index) const {
    for(const auto& pass : _passes) {
        if(pass->_index == pass_index) {
            return pass->name();
        }
    }
    y_fatal("Pass index out of bounds ({})", pass_index);
}

FrameGraphMutableImageId FrameGraph::declare_image(ImageFormat format, const math::Vec2ui& size) {
    y_debug_assert(size.x() > 0 && size.y() > 0);
    const u32 id = _resources->create_image_id();

    _images.set_min_size(id + 1);

    auto& [i, r] = _images[id];
    y_always_assert(!i.is_valid(), "Resource already exists");
    i._id = id;
    r.size.to<2>() = size;
    r.format = format;

    return i;
}

FrameGraphMutableVolumeId FrameGraph::declare_volume(ImageFormat format, const math::Vec3ui& size) {
    y_debug_assert(size.x() > 0 && size.y() > 0 && size.z() > 0);
    const u32 id = _resources->create_volume_id();

    _volumes.set_min_size(id + 1);

    auto& [i, r] = _volumes[id];
    y_always_assert(!i.is_valid(), "Resource already exists");
    i._id = id;
    r.size = size;
    r.format = format;

    return i;
}

FrameGraphMutableBufferId FrameGraph::declare_buffer(u64 byte_size) {
    y_debug_assert(byte_size > 0);
    const u32 id = _resources->create_buffer_id();

    _buffers.set_min_size(id + 1);

    auto& [i, r] = _buffers[id];
    y_always_assert(!i.is_valid(), "Resource already exists");
    i._id = id;
    r.byte_size = byte_size;

    return i;
}

FrameGraphPass* FrameGraph::create_pass(std::string_view name) {
    auto pass = std::make_unique<FrameGraphPass>(name, this, ++_pass_index);
    FrameGraphPass* ptr = pass.get();
     _passes << std::move(pass);
     return ptr;
}

FrameGraphPassBuilder FrameGraph::add_pass(std::string_view name) {
    return FrameGraphPassBuilder(create_pass(name));
}

FrameGraphComputePassBuilder FrameGraph::add_compute_pass(std::string_view name) {
     return FrameGraphComputePassBuilder(create_pass(name));
}

const FrameGraph::ImageCreateInfo& FrameGraph::info(FrameGraphImageId res) const {
    return check_exists(_images, res);
}

const FrameGraph::ImageCreateInfo& FrameGraph::info(FrameGraphVolumeId res) const {
    return check_exists(_volumes, res);
}

const FrameGraph::BufferCreateInfo& FrameGraph::info(FrameGraphBufferId res) const {
    return check_exists(_buffers, res);
}

bool FrameGraph::ResourceCreateInfo::is_persistent() const {
    return persistent.is_valid();
}

bool FrameGraph::ResourceCreateInfo::is_prev() const {
    return persistent_prev.is_valid();
}

usize FrameGraph::ResourceCreateInfo::last_use() const {
    return std::max(last_read, last_write);
}

void FrameGraph::ResourceCreateInfo::register_use(usize index, bool is_written) {
    y_debug_assert(!is_written || !is_prev());
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
    y_debug_assert(!is_persistent());

    last_write = std::max(last_write, other.last_write);
    last_read = std::max(last_read, other.last_read);
    usage = usage | other.usage;
    last_usage = last_usage | other.last_usage;
    persistent = other.persistent;
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

    y_always_assert(!info.is_prev() || (info.usage & usage) == usage, "Persistent resources from previous frames can not get additional usage flags");
    info.usage = info.usage | usage;

    const bool is_last = info.last_use() < pass->_index;
    info.last_usage = is_last ? usage : (info.last_usage | usage);
    info.register_use(pass->_index, is_written);
}

void FrameGraph::register_usage(FrameGraphVolumeId res, ImageUsage usage, bool is_written, const FrameGraphPass* pass) {
    check_usage_io(usage, is_written);
    auto& info = check_exists(_volumes, res);

    y_always_assert(!info.is_prev() || (info.usage & usage) == usage, "Persistent resources from previous frames can not get additional usage flags");
    info.usage = info.usage | usage;

    const bool is_last = info.last_use() < pass->_index;
    info.last_usage = is_last ? usage : (info.last_usage | usage);
    info.register_use(pass->_index, is_written);
}

void FrameGraph::register_usage(FrameGraphBufferId res, BufferUsage usage, bool is_written, const FrameGraphPass* pass) {
    check_usage_io(usage, is_written);
    auto& info = check_exists(_buffers, res);

    y_always_assert(!info.is_prev() || (info.usage & usage) == usage, "Persistent resources from previous frames can not get additional usage flags");
    info.usage = info.usage | usage;

    info.register_use(pass->_index, is_written);
}

void FrameGraph::register_image_copy(FrameGraphMutableImageId dst, FrameGraphImageId src, const FrameGraphPass* pass) {
    auto& info = check_exists(_images, dst);
    y_always_assert(!info.copy_src.is_valid(), "Resource is already a copy");
    info.copy_src = src;
    _image_copies.push_back({pass->_index, dst, src});
}

void FrameGraph::register_buffer_copy(FrameGraphMutableBufferId dst, FrameGraphBufferId src, const FrameGraphPass* pass) {
    _buffer_copies.push_back({pass->_index, dst, src});
}

void FrameGraph::register_image_clear(FrameGraphMutableImageId dst, const FrameGraphPass* pass) {
    check_exists(_images, dst);
    _image_clears.push_back({pass->_index, dst});
}

template<typename C, typename P, typename T, typename U>
static void make_persistent(C& c, P& persistents, T res, U persistent_id) {
    persistent_id.check_valid();
    auto& info = check_exists(c, res);
    y_always_assert(!info.persistent.is_valid(), "Resource already has a persistent ID");
    info.persistent = persistent_id;

    persistents.set_min_size(persistent_id.id() + 1);
    auto& p = persistents[persistent_id.id()];
    y_always_assert(!p.is_valid(), "Persistent id has already been registered");
    p = res;
}

template<typename C, typename T, typename U>
static auto& alloc_prev(C& c, T new_res, U persistent_id) {
    c.set_min_size(new_res.id() + 1);
    auto& [i, info]  = c[new_res.id()];
    i = new_res;
    info.persistent_prev = persistent_id;
    return info;
}

FrameGraphImageId FrameGraph::make_persistent_and_get_prev(FrameGraphImageId res, FrameGraphPersistentResourceId persistent_id) {
    make_persistent(_images, _persistents, res, persistent_id);

    if(!_resources->has_prev_image(persistent_id)) {
        return {};
    }

    FrameGraphMutableImageId id;
    id._id = _resources->create_image_id();

    _resources->create_prev_image(id, persistent_id);
    auto& info = alloc_prev(_images, id, persistent_id);

    const auto& image = _resources->find(id);
    info.format = image.format();
    info.size = image.image_size();
    info.usage = image.usage();

    return id;
}

FrameGraphBufferId FrameGraph::make_persistent_and_get_prev(FrameGraphBufferId res, FrameGraphPersistentResourceId persistent_id) {
    make_persistent(_buffers, _persistents, res, persistent_id);

    if(!_resources->has_prev_buffer(persistent_id)) {
        return {};
    }

    FrameGraphMutableBufferId id;
    id._id = _resources->create_buffer_id();

    _resources->create_prev_buffer(id, persistent_id);
    auto& info = alloc_prev(_buffers, id, persistent_id);

    const auto& buffer = _resources->find(id);
    info.byte_size = buffer.byte_size();
    info.usage = buffer.usage();

    return id;
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

void FrameGraph::map_buffer(FrameGraphMutableBufferId res, const FrameGraphPass* pass) {
    auto& info = check_exists(_buffers, res);
    info.usage = info.usage | BufferUsage::TransferDstBit;
    info.memory_type = MemoryType::Staging;
    info.register_use(pass->_index, true);
}

bool FrameGraph::is_attachment(FrameGraphImageId res) const {
    const auto& info = check_exists(_images, res);
    return (info.usage & ImageUsage::Attachment) != ImageUsage::None;
}

math::Vec2ui FrameGraph::image_size(FrameGraphImageId res) const {
    const auto& info = check_exists(_images, res);
    return info.size.to<2>();
}

math::Vec3ui FrameGraph::volume_size(FrameGraphVolumeId res) const {
    const auto& info = check_exists(_volumes, res);
    return info.size;
}

ImageFormat FrameGraph::image_format(FrameGraphImageId res) const {
    const auto& info = check_exists(_images, res);
    return info.format;
}

ImageFormat FrameGraph::volume_format(FrameGraphVolumeId res) const {
    const auto& info = check_exists(_volumes, res);
    return info.format;
}

u64 FrameGraph::buffer_byte_size(FrameGraphBufferId res) const {
    const auto& info = check_exists(_buffers, res);
    return info.byte_size;
}

}

