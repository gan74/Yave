/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#include "DescriptorSet.h"

#include <yave/Device.h>

#include <unordered_map>

namespace yave {

static vk::DescriptorPool create_descriptor_pool(DevicePtr dptr, const std::unordered_map<vk::DescriptorType, u32>& binding_counts) {
	auto sizes = core::range(binding_counts).map([](const auto& count) {
			return vk::DescriptorPoolSize()
					.setType(count.first)
					.setDescriptorCount(count.second)
				;
		}).collect<core::Vector>();

	return dptr->vk_device().createDescriptorPool(vk::DescriptorPoolCreateInfo()
			.setPoolSizeCount(sizes.size())
			.setPPoolSizes(sizes.begin())
			.setMaxSets(1)
		);
}

static vk::DescriptorSet create_descriptor_set(DevicePtr dptr, vk::DescriptorPool pool, vk::DescriptorSetLayout layout) {
	return dptr->vk_device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(pool)
			.setDescriptorSetCount(1)
			.setPSetLayouts(&layout)
		).front();
}

static void update_sets(DevicePtr dptr, vk::DescriptorSet set, const core::Vector<vk::DescriptorSetLayoutBinding>& /*layout_binding*/, const core::Vector<Binding>& bindings) {
	auto writes = core::Vector<vk::WriteDescriptorSet>();
	for(const auto& binding : bindings) {
		auto w = vk::WriteDescriptorSet()
				.setDstSet(set)
				.setDstBinding(u32(writes.size()))
				.setDstArrayElement(0)
				.setDescriptorCount(1)
				.setDescriptorType(binding.vk_descriptor_type())
			;

		if(binding.is_buffer()) {
			w.setPBufferInfo(&binding.descriptor_info().buffer);
		} else if(binding.is_image()) {
			w.setPImageInfo(&binding.descriptor_info().image);
		} else {
			fatal("Unknown descriptor type");
		}

		writes << w;
	}
	dptr->vk_device().updateDescriptorSets(u32(writes.size()), writes.begin(), 0, nullptr);
}




DescriptorSet::DescriptorSet(DevicePtr dptr, std::initializer_list<Binding> bindings) : DescriptorSet(dptr, core::vector(bindings)) {
}

DescriptorSet::DescriptorSet(DevicePtr dptr, const core::Vector<Binding>& bindings) : DeviceLinked(dptr) {
	if(!bindings.is_empty()) {
		auto layout_bindings = core::Vector<vk::DescriptorSetLayoutBinding>();

		std::unordered_map<vk::DescriptorType, u32> binding_counts;
		for(const auto& binding : bindings) {
			layout_bindings << binding.descriptor_set_layout_binding(layout_bindings.size());
			binding_counts[binding.vk_descriptor_type()]++;
		}

		auto layout = dptr->create_descriptor_set_layout(layout_bindings);
		_pool = create_descriptor_pool(dptr, binding_counts);
		_set = create_descriptor_set(dptr, _pool, layout);
		update_sets(dptr, _set, layout_bindings, bindings);
	}
}

DescriptorSet::~DescriptorSet() {
	destroy(_pool);
}

DescriptorSet::DescriptorSet(DescriptorSet&& other) {
	swap(other);
}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other) {
	swap(other);
	return *this;
}

const vk::DescriptorSet& DescriptorSet::vk_descriptor_set() const {
	return _set;
}

/*const vk::DescriptorSetLayout& DescriptorSet::vk_descriptor_set_layout() const {
	return _layout;
}*/

void DescriptorSet::swap(DescriptorSet& other) {
	DeviceLinked::swap(other);
	std::swap(_pool, other._pool);
	//std::swap(_layout, other._layout);
	std::swap(_set, other._set);
}


}
