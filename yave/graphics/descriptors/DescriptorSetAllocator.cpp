/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

DescriptorSetLayout::DescriptorSetLayout(DevicePtr dptr, core::Span<vk::DescriptorSetLayoutBinding> bindings) : DeviceLinked(dptr) {
	for(const auto& d : bindings) {
		++_sizes[usize(d.descriptorType)];
	}
	_layout = dptr->vk_device().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(u32(bindings.size()))
			.setPBindings(bindings.begin())
		);
}

DescriptorSetLayout::~DescriptorSetLayout() {
	destroy(_layout);
}

const math::Vec<DescriptorSetLayout::max_descriptor_type, u32>& DescriptorSetLayout::desciptors_count() const {
	return _sizes;
}

vk::DescriptorSetLayout DescriptorSetLayout::vk_descriptor_set_layout() const {
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

vk::DescriptorSet DescriptorSetData::vk_descriptor_set() const {
	return _pool->vk_descriptor_set(_index);
}


static vk::DescriptorPool create_descriptor_pool(const DescriptorSetLayout& layout, usize set_count) {
	y_profile();

	usize sizes_count = 0;
	std::array<vk::DescriptorPoolSize, DescriptorSetLayout::max_descriptor_type> sizes;

	for(usize i = 0; i != DescriptorSetLayout::max_descriptor_type; ++i) {
		usize type_count = layout.desciptors_count()[i];
		if(type_count) {
			sizes[sizes_count++]
					.setType(vk::DescriptorType(i))
					.setDescriptorCount(type_count * set_count)
				;
		}
	}

	if(!sizes_count) {
		return {};
	}

	return layout.device()->vk_device().createDescriptorPool(vk::DescriptorPoolCreateInfo()
			.setPoolSizeCount(sizes_count)
			.setPPoolSizes(sizes.begin())
			.setMaxSets(set_count)
		);
}

DescriptorSetPool::DescriptorSetPool(const DescriptorSetLayout& layout) :
	DeviceLinked(layout.device()),
	_pool(create_descriptor_pool(layout, pool_size)) {

	std::array<vk::DescriptorSetLayout, pool_size> layouts;
	std::fill_n(layouts.begin(), pool_size, layout.vk_descriptor_set_layout());

	auto create_info = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(_pool)
			.setDescriptorSetCount(pool_size)
			.setPSetLayouts(layouts.begin())
		;

	device()->vk_device().allocateDescriptorSets(&create_info, _sets.begin());
}

DescriptorSetPool::~DescriptorSetPool() {
	y_debug_assert(_taken.none());
	destroy(_pool);
}

DescriptorSetData DescriptorSetPool::alloc() {
	y_profile();
	auto lock = std::unique_lock(_lock);
	if(is_full() || _taken[_first_free]) {
		y_fatal("DescriptorSetPoolPage is full.");
	}
	u32 id = _first_free;

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
	auto lock = std::unique_lock(_lock);
	y_debug_assert(_taken[id]);
	_taken.reset(id);
	_first_free = std::min(_first_free, id);
}

bool DescriptorSetPool::is_full() const {
	// we shouldn't need to lock here
	return _first_free >= pool_size;
}

vk::DescriptorSet DescriptorSetPool::vk_descriptor_set(u32 id) const {
	return _sets[id];
}

vk::DescriptorPool DescriptorSetPool::vk_pool() const {
	return _pool;
}

usize DescriptorSetPool::free_sets() const {
	return pool_size - used_sets();
}

usize DescriptorSetPool::used_sets() const {
	auto lock = std::unique_lock(_lock);
	return _taken.count();
}



DescriptorSetAllocator::DescriptorSetAllocator(DevicePtr dptr) : DeviceLinked(dptr) {
}

DescriptorSetData DescriptorSetAllocator::create_descritptor_set(const Key& bindings) {
	y_profile();
	auto lock = std::unique_lock(_lock);
	auto& pool = layout(bindings);

	auto reversed = core::Range(std::make_reverse_iterator(pool.pools.end()),
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
	auto lock = std::unique_lock(_lock);
	return layout(bindings).layout;
}

DescriptorSetAllocator::Pools& DescriptorSetAllocator::layout(const Key& bindings) {
	auto& layout  = _layouts[bindings];
	if(layout.layout.is_null()) {
		layout.layout = DescriptorSetLayout(device(), bindings);
	}
	return layout;
}

usize DescriptorSetAllocator::layout_count() const {
	auto lock = std::unique_lock(_lock);
	return _layouts.size();
}

usize DescriptorSetAllocator::pool_count() const {
	auto lock = std::unique_lock(_lock);
	usize count = 0;
	for(const auto& l : _layouts) {
		count += l.second.pools.size();
	}
	return count;
}

usize DescriptorSetAllocator::free_sets() const {
	auto lock = std::unique_lock(_lock);
	usize count = 0;
	for(const auto& l : _layouts) {
		for(const auto& p : l.second.pools) {
			count += p->free_sets();
		}
	}
	return count;
}

usize DescriptorSetAllocator::used_sets() const {
	auto lock = std::unique_lock(_lock);
	usize count = 0;
	for(const auto& l : _layouts) {
		for(const auto& p : l.second.pools) {
			count += p->used_sets();
		}
	}
	return count;
}

}
