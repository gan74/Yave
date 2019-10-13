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

#include "DescriptorSet.h"

#include <yave/device/Device.h>

#include <unordered_map>

namespace yave {

static constexpr usize max_descriptor_type = std::max({
		usize(vk::DescriptorType::eSampler),
		usize(vk::DescriptorType::eCombinedImageSampler),
		usize(vk::DescriptorType::eSampledImage),
		usize(vk::DescriptorType::eStorageImage),
		usize(vk::DescriptorType::eUniformTexelBuffer),
		usize(vk::DescriptorType::eStorageTexelBuffer),
		usize(vk::DescriptorType::eUniformBuffer),
		usize(vk::DescriptorType::eStorageBuffer),
		usize(vk::DescriptorType::eUniformBufferDynamic),
		usize(vk::DescriptorType::eStorageBufferDynamic)
	}) + 1;

static vk::DescriptorPool create_descriptor_pool(DevicePtr dptr, const std::array<u32, max_descriptor_type>& binding_counts) {
	usize sizes_count = 0;
	std::array<vk::DescriptorPoolSize, max_descriptor_type> sizes;

	for(usize i = 0; i != binding_counts.size(); ++i) {
		if(binding_counts[i]) {
			sizes[sizes_count++]
					.setType(vk::DescriptorType(i))
					.setDescriptorCount(binding_counts[i])
				;
		}
	}

	return dptr->vk_device().createDescriptorPool(vk::DescriptorPoolCreateInfo()
			.setPoolSizeCount(sizes_count)
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

static void update_sets(DevicePtr dptr, vk::DescriptorSet set, core::Span<Binding> bindings) {
	auto writes = core::vector_with_capacity<vk::WriteDescriptorSet>(bindings.size());
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
			y_fatal("Unknown descriptor type.");
		}

		writes << w;
	}
	dptr->vk_device().updateDescriptorSets(u32(writes.size()), writes.begin(), 0, nullptr);
}




DescriptorSet::DescriptorSet(DevicePtr dptr, core::Span<Binding> bindings) : DeviceLinked(dptr) {
	if(!bindings.is_empty()) {
		auto layout_bindings = core::vector_with_capacity<vk::DescriptorSetLayoutBinding>(bindings.size());

		std::array<u32, max_descriptor_type> binding_counts = {};
		for(const auto& binding : bindings) {
			layout_bindings << binding.descriptor_set_layout_binding(layout_bindings.size());
			++binding_counts[usize(binding.vk_descriptor_type())];
		}

		auto layout = dptr->create_descriptor_set_layout(layout_bindings);
		_pool = create_descriptor_pool(dptr, binding_counts);
		_set = create_descriptor_set(dptr, _pool, layout);
		update_sets(dptr, _set, bindings);
	}
}

DescriptorSet::~DescriptorSet() {
	destroy(_pool);
}

}
