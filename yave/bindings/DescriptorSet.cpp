/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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

#include "DescriptorSet.h"

#include <yave/device/Device.h>

#include <unordered_map>

namespace yave {

static vk::DescriptorPool create_descriptor_pool(DevicePtr dptr, const std::unordered_map<vk::DescriptorType, u32>& binding_counts) {
	auto sizes = core::vector_with_capacity<vk::DescriptorPoolSize>(binding_counts.size());
	std::transform(binding_counts.begin(), binding_counts.end(), std::back_inserter(sizes), [](const auto& count) {
			return vk::DescriptorPoolSize()
					.setType(count.first)
					.setDescriptorCount(count.second)
				;
		});


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

static void update_sets(DevicePtr dptr, vk::DescriptorSet set, const core::Vector<vk::DescriptorSetLayoutBinding>& /*layout_binding*/, const core::ArrayView<Binding>& bindings) {
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
			fatal("Unknown descriptor type.");
		}

		writes << w;
	}
	dptr->vk_device().updateDescriptorSets(u32(writes.size()), writes.begin(), 0, nullptr);
}




DescriptorSet::DescriptorSet(DevicePtr dptr, const core::ArrayView<Binding>& bindings) : DeviceLinked(dptr) {
	if(!bindings.is_empty()) {
		auto layout_bindings = core::Vector<vk::DescriptorSetLayoutBinding>();

		std::unordered_map<vk::DescriptorType, u32> binding_counts;
		for(const auto& binding : bindings) {
			layout_bindings << binding.descriptor_set_layout_binding(layout_bindings.size());
			++binding_counts[binding.vk_descriptor_type()];
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

void DescriptorSet::swap(DescriptorSet& other) {
	DeviceLinked::swap(other);
	std::swap(_pool, other._pool);
	std::swap(_set, other._set);
}


}
