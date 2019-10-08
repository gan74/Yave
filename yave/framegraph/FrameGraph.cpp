/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include <yave/utils/color.h>

namespace yave {

template<typename U>
static bool is_none(U u) {
	return u == U::None;
}

template<typename T, typename C>
static auto&& check_exists(C& c, T t) {
	auto it = c.find(t);
	if(it == c.end()) {
		y_fatal("Resource doesn't exist.");
	}
	return it->second;
}

template<typename C, typename B>
static void build_barriers(const C& resources, B& barriers, std::unordered_map<FrameGraphResourceId, PipelineStage>& to_barrier, FrameGraphResourcePool* pool) {
	for(auto&& [res, info] : resources) {
		auto it = to_barrier.find(res);
		bool exists = it != to_barrier.end();
		// barrier around attachments are handled by the renderpass
		PipelineStage stage = info.stage & ~PipelineStage::AllAttachmentOutBit;
		if(stage == PipelineStage::None) {
			if(exists) {
				to_barrier.erase(it);
			}
			continue;
		}

		if(exists) {
			barriers.emplace_back(pool->barrier(res, it->second, info.stage));
			it->second = info.stage;
		} else {
			to_barrier[res] = info.stage;
		}
	}
}

static void copy_images(CmdBufferRecorder& recorder, core::Span<std::pair<FrameGraphImageId, FrameGraphMutableImageId>> copies,
						std::unordered_map<FrameGraphResourceId, PipelineStage>& to_barrier, FrameGraphResourcePool* pool) {

	Y_TODO(We might end up barriering twice here)
	for(auto [src, dst] : copies) {
		if(pool->are_aliased(src, dst)) {
			if(auto it = to_barrier.find(src); it != to_barrier.end()) {
				to_barrier[dst] = it->second;
				to_barrier.erase(it);
			}
		} else {
			to_barrier.erase(src);
			to_barrier.erase(dst);
			recorder.barriered_copy(pool->image_base(src), pool->image_base(dst));
		}
	}
}


FrameGraph::FrameGraph(const std::shared_ptr<FrameGraphResourcePool>& pool) : _pool(pool) {
}

DevicePtr FrameGraph::device() const {
	return _pool->device();
}

const FrameGraphResourcePool* FrameGraph::resources() const {
	return _pool.get();
}

void FrameGraph::render(CmdBufferRecorder& recorder) && {
	y_profile();
	Y_TODO(Pass culling)

	alloc_resources();

	std::unordered_map<FrameGraphResourceId, PipelineStage> to_barrier;
	core::Vector<BufferBarrier> buffer_barriers;
	core::Vector<ImageBarrier> image_barriers;

	usize pass_id = 0;
	for(const auto& pass : _passes) {
		y_profile_zone(pass->name());
		auto region = recorder.region(pass->name(), math::Vec4(identifying_color(pass_id++), 1.0f));

		{
			y_profile_zone("prepare");
			copy_images(recorder, pass->_image_copies, to_barrier, _pool.get());
		}

		{
			y_profile_zone("init");
			pass->init_framebuffer(_pool.get());
			pass->init_descriptor_sets(_pool.get());
		}

		{
			y_profile_zone("barriers");
			buffer_barriers.make_empty();
			image_barriers.make_empty();
			build_barriers(pass->_buffers, buffer_barriers, to_barrier, _pool.get());
			build_barriers(pass->_images, image_barriers, to_barrier, _pool.get());
			recorder.barriers(buffer_barriers, image_barriers);
		}

		{
			y_profile_zone("render");
			pass->render(recorder);
		}
	}


	Y_TODO(put resource barriers at the end of the graph to prevent clash with whatever comes after)

	release_resources(recorder);
}

void FrameGraph::alloc_resources() {
	y_profile();
	for(auto&& [res, info] : _images) {
		if(is_none(info.usage)) {
			y_fatal("Unused frame graph image resource.");
		}
		_pool->create_image(res, info.format, info.size, info.usage);
	}
	for(auto&& [res, info] : _buffers) {
		if(is_none(info.usage)) {
			y_fatal("Unused frame graph buffer resource.");
		}
		_pool->create_buffer(res, info.byte_size, info.usage, info.memory_type);
	}
}

void FrameGraph::release_resources(CmdBufferRecorder& recorder) {
	y_profile();
	struct BufferRelease : NonCopyable {
		FrameGraphBufferId res;
		FrameGraphResourcePool* pool = nullptr;

		BufferRelease(FrameGraphBufferId r, FrameGraphResourcePool* p) : res(r), pool(p) {
		}

		BufferRelease(BufferRelease&& other) {
			std::swap(other.res, res);
			std::swap(other.pool, pool);
		}

		~BufferRelease() {
			if(pool) {
				pool->release(res);
			}
		}
	};


	auto buffers = core::vector_with_capacity<BufferRelease>(_buffers.size());
	std::transform(_buffers.begin(), _buffers.end(), std::back_inserter(buffers), [=](const auto& buff) { return BufferRelease(buff.first, _pool.get()) ; });
	recorder.keep_alive(std::pair{_pool, std::move(buffers)});

	for(auto&& i : _images) {
		_pool->release(i.first);
	}
}

FrameGraphMutableImageId FrameGraph::declare_image(ImageFormat format, const math::Vec2ui& size) {
	FrameGraphMutableImageId res;
	res._id = _pool->create_resource_id();
	auto& r = _images[res];
	r.size = size;
	r.format = format;
	return res;
}

FrameGraphMutableBufferId FrameGraph::declare_buffer(usize byte_size) {
	FrameGraphMutableBufferId res;
	res._id = _pool->create_resource_id();
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

void FrameGraph::ResourceCreateInfo::register_use(usize index) {
	if(!first_use) {
		first_use = index;
	}
	last_use = std::max(last_use,index);
}

void FrameGraph::register_usage(FrameGraphImageId res, ImageUsage usage, const FrameGraphPass* pass) {
	auto& info = check_exists(_images, res);
	info.usage = info.usage | usage;
	info.register_use(pass->_index);
}

void FrameGraph::register_usage(FrameGraphBufferId res, BufferUsage usage, const FrameGraphPass* pass) {
	auto& info = check_exists(_buffers, res);
	info.usage = info.usage | usage;
	info.register_use(pass->_index);
}

void FrameGraph::set_cpu_visible(FrameGraphMutableBufferId res, const FrameGraphPass*pass) {
	auto& info = check_exists(_buffers, res);
	info.memory_type = MemoryType::CpuVisible;
	info.register_use(pass->_index);
}

bool FrameGraph::is_attachment(FrameGraphImageId res) const {
	const auto& info = check_exists(_images, res);
	return (info.usage & ImageUsage::Attachment) != ImageUsage::None;
}

math::Vec2ui FrameGraph::image_size(FrameGraphImageId res) const {
	const auto& info = check_exists(_images, res);
	return info.size;
}


}
