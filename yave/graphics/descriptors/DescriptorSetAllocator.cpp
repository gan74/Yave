/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include "DescriptorSetAllocator.h"
#include "Descriptor.h"

namespace yave {

static constexpr usize inline_block_index = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1;

static usize descriptor_type_index(VkDescriptorType type) {
	if(type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT) {
		y_debug_assert(inline_block_index < DescriptorSetLayout::descriptor_type_count);
		return inline_block_index;
	}

	y_debug_assert(usize(type) < DescriptorSetLayout::descriptor_type_count);
	return usize(type);
}

static VkDescriptorType index_descriptor_type(usize index) {
	y_debug_assert(index <  DescriptorSetLayout::descriptor_type_count);
	if(index == inline_block_index) {
		return VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT;
	}
	return VkDescriptorType(index);
}




DescriptorSetLayout::DescriptorSetLayout(DevicePtr dptr, core::Span<VkDescriptorSetLayoutBinding> bindings) : DeviceLinked(dptr) {
	for(const auto& d : bindings) {
		_sizes[descriptor_type_index(d.descriptorType)] += d.descriptorCount;
		y_always_assert(d.descriptorType != VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT || dptr->device_properties().max_inline_uniform_size != 0, "Inline descriptors are not supported");
	}

	VkDescriptorSetLayoutCreateInfo create_info = vk_struct();
	{
		create_info.bindingCount = bindings.size();
		create_info.pBindings = bindings.data();
	}
	vk_check(vkCreateDescriptorSetLayout(dptr->vk_device(), &create_info, nullptr, &_layout.get()));
}

DescriptorSetLayout::~DescriptorSetLayout() {
	destroy(_layout);
}

const std::array<u32, DescriptorSetLayout::descriptor_type_count>& DescriptorSetLayout::desciptors_count() const {
	return _sizes;
}

VkDescriptorSetLayout DescriptorSetLayout::vk_descriptor_set_layout() const {
	return _layout;
}




DescriptorSetData::DescriptorSetData(DescriptorSetPool* pool, u32 id) : _pool(pool), _index(id) {
}

DevicePtr DescriptorSetData::device() const {
	return _pool ? _pool->device() : nullptr;
}

bool DescriptorSetData::is_null() const {
	return !device();
}

void DescriptorSetData::recycle() {
	if(_pool) {
		_pool->recycle(_index);
	}
}

VkDescriptorSetLayout DescriptorSetData::vk_descriptor_set_layout() const {
	return _pool->vk_descriptor_set_layout();
}

VkDescriptorSet DescriptorSetData::vk_descriptor_set() const {
	return _pool->vk_descriptor_set(_index);
}


static VkDescriptorPool create_descriptor_pool(const DescriptorSetLayout& layout, usize set_count) {
	y_profile();

	usize sizes_count = 0;
	std::array<VkDescriptorPoolSize, DescriptorSetLayout::descriptor_type_count> sizes;

	for(usize i = 0; i != DescriptorSetLayout::descriptor_type_count; ++i) {
		const usize type_count = layout.desciptors_count()[i];
		if(type_count) {
			VkDescriptorPoolSize& pool_size = sizes[sizes_count++];
			pool_size.type = index_descriptor_type(i);
			pool_size.descriptorCount = type_count * set_count;
		}
	}

	if(!sizes_count) {
		return {};
	}

	VkDescriptorPoolCreateInfo create_info = vk_struct();
	{
		create_info.poolSizeCount = sizes_count;
		create_info.pPoolSizes = sizes.data();
		create_info.maxSets = set_count;
	}

	VkDescriptorPool pool = {};
	vk_check(vkCreateDescriptorPool(layout.device()->vk_device(), &create_info, nullptr, &pool));
	return pool;
}

DescriptorSetPool::DescriptorSetPool(const DescriptorSetLayout& layout) :
	DeviceLinked(layout.device()),
	_pool(create_descriptor_pool(layout, pool_size)),
	_layout(layout.vk_descriptor_set_layout()) {

	std::array<VkDescriptorSetLayout, pool_size> layouts;
	std::fill_n(layouts.begin(), pool_size, layout.vk_descriptor_set_layout());

	VkDescriptorSetAllocateInfo allocate_info = vk_struct();
	{
		allocate_info.descriptorPool = _pool;
		allocate_info.descriptorSetCount = pool_size;
		allocate_info.pSetLayouts = layouts.data();
	}

	vk_check(vkAllocateDescriptorSets(device()->vk_device(), &allocate_info, _sets.data()));
}

DescriptorSetPool::~DescriptorSetPool() {
	y_debug_assert(_taken.none());
	destroy(_pool);
}

DescriptorSetData DescriptorSetPool::alloc() {
	y_profile();
	const auto lock = y_profile_unique_lock(_lock);
	if(is_full() || _taken[_first_free]) {
		y_fatal("DescriptorSetPoolPage is full.");
	}
	const u32 id = _first_free;

	for(++_first_free; _first_free < pool_size; ++_first_free) {
		if(!_taken[_first_free]) {
			break;
		}
	}

	y_debug_assert(is_full() || !_taken[_first_free]);

	_taken.set(id);
	return DescriptorSetData(this, id);
}

void DescriptorSetPool::recycle(u32 id) {
	y_profile();
	const auto lock = y_profile_unique_lock(_lock);
	y_debug_assert(_taken[id]);
	_taken.reset(id);
	_first_free = std::min(_first_free, id);
}

bool DescriptorSetPool::is_full() const {
	// we shouldn't need to lock here
	return _first_free >= pool_size;
}

VkDescriptorSet DescriptorSetPool::vk_descriptor_set(u32 id) const {
	return _sets[id];
}

VkDescriptorPool DescriptorSetPool::vk_pool() const {
	return _pool;
}

VkDescriptorSetLayout DescriptorSetPool::vk_descriptor_set_layout() const {
	return _layout;
}

usize DescriptorSetPool::free_sets() const {
	return pool_size - used_sets();
}

usize DescriptorSetPool::used_sets() const {
	const auto lock = y_profile_unique_lock(_lock);
	return _taken.count();
}



DescriptorSetAllocator::DescriptorSetAllocator(DevicePtr dptr) : DeviceLinked(dptr) {
}

DescriptorSetData DescriptorSetAllocator::create_descritptor_set(const Key& bindings) {
	y_profile();
	const auto lock = y_profile_unique_lock(_lock);
	auto& pool = layout(bindings);

	const auto reversed = core::Range(std::make_reverse_iterator(pool.pools.end()),
									  std::make_reverse_iterator(pool.pools.begin()));
	for(auto& page : reversed) {
		if(!page->is_full()) {
			return page->alloc();
		}
	}

	pool.pools.emplace_back(std::make_unique<DescriptorSetPool>(pool.layout));
	return pool.pools.last()->alloc();
}

const DescriptorSetLayout& DescriptorSetAllocator::descriptor_set_layout(const Key& bindings) {
	const auto lock = y_profile_unique_lock(_lock);
	return layout(bindings).layout;
}

DescriptorSetAllocator::LayoutPools& DescriptorSetAllocator::layout(const Key& bindings) {
	auto& layout  = _layouts[bindings];
	if(layout.layout.is_null()) {
		layout.layout = DescriptorSetLayout(device(), bindings);
	}
	return layout;
}

usize DescriptorSetAllocator::layout_count() const {
	const auto lock = y_profile_unique_lock(_lock);
	return _layouts.size();
}

usize DescriptorSetAllocator::pool_count() const {
	const auto lock = y_profile_unique_lock(_lock);
	usize count = 0;
	for(const auto& l : _layouts) {
		count += l.second.pools.size();
	}
	return count;
}

usize DescriptorSetAllocator::free_sets() const {
	const auto lock = y_profile_unique_lock(_lock);
	usize count = 0;
	for(const auto& l : _layouts) {
		for(const auto& p : l.second.pools) {
			count += p->free_sets();
		}
	}
	return count;
}

usize DescriptorSetAllocator::used_sets() const {
	const auto lock = y_profile_unique_lock(_lock);
	usize count = 0;
	for(const auto& l : _layouts) {
		for(const auto& p : l.second.pools) {
			count += p->used_sets();
		}
	}
	return count;
}

}
