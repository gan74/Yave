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

namespace yave {

static vk::DescriptorPool create_descriptor_pool(DevicePtr dptr) {
	auto ub_pool_size = vk::DescriptorPoolSize()
			.setDescriptorCount(1)
			.setType(vk::DescriptorType::eUniformBuffer)
		;

	auto tx_pool_size = vk::DescriptorPoolSize()
			.setDescriptorCount(1)
			.setType(vk::DescriptorType::eCombinedImageSampler)
		;

	vk::DescriptorPoolSize sizes[] = {ub_pool_size, tx_pool_size};
	return dptr->get_vk_device().createDescriptorPool(vk::DescriptorPoolCreateInfo()
			.setPoolSizeCount(2)
			.setPPoolSizes(sizes)
			.setMaxSets(1)
		);
}

static vk::DescriptorSet create_descriptor_set(DevicePtr dptr, vk::DescriptorPool pool, vk::DescriptorSetLayout layout) {
	return dptr->get_vk_device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo()
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
				.setDstBinding(writes.size())
				.setDstArrayElement(0)
				.setDescriptorCount(1)
				.setDescriptorType(binding.get_vk_descriptor_type())
			;

		switch(binding.get_vk_descriptor_type()) {
			case vk::DescriptorType::eCombinedImageSampler:
				w.setPImageInfo(&binding.descriptor_info().image);
				break;

			case vk::DescriptorType::eUniformBuffer:
				w.setPBufferInfo(&binding.descriptor_info().buffer);
				break;

			default:
				fatal("Unknown descriptor type");
				break;
		}
		writes << w;
	}
	dptr->get_vk_device().updateDescriptorSets(writes.size(), writes.begin(), 0, nullptr);
}




DescriptorSet::DescriptorSet(DevicePtr dptr, std::initializer_list<Binding> bindings) : DescriptorSet(dptr, core::vector(bindings)) {
}

DescriptorSet::DescriptorSet(DevicePtr dptr, const core::Vector<Binding>& bindings) : DeviceLinked(dptr) {
	auto layout_bindings = core::Vector<vk::DescriptorSetLayoutBinding>();
	for(const auto& binding : bindings) {
		layout_bindings << binding.descriptor_set_layout_binding(layout_bindings.size());
	}
	_layout = dptr->get_vk_device().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(layout_bindings.size())
			.setPBindings(layout_bindings.begin())
		);

	_pool = create_descriptor_pool(dptr);
	_set = create_descriptor_set(dptr, _pool, _layout);
	update_sets(dptr, _set, layout_bindings, bindings);

}

DescriptorSet::DescriptorSet(DescriptorSet&& other) {
	swap(other);
}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other) {
	swap(other);
	return *this;
}

const vk::DescriptorSet& DescriptorSet::get_vk_descriptor_set() const {
	return _set;
}

const vk::DescriptorSetLayout& DescriptorSet::get_vk_descriptor_set_layout() const {
	return _layout;
}

void DescriptorSet::swap(DescriptorSet& other) {
	DeviceLinked::swap(other);
	std::swap(_pool, other._pool);
	std::swap(_layout, other._layout);
	std::swap(_set, other._set);
}


}
