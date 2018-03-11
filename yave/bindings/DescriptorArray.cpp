/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include "DescriptorArray.h"

#include <yave/device/Device.h>

#include <unordered_map>

namespace yave {

static vk::DescriptorPool create_descriptor_array_pool(DevicePtr dptr, u32 count, vk::DescriptorType type) {
	vk::DescriptorPoolSize pool_size = vk::DescriptorPoolSize()
			.setType(type)
			.setDescriptorCount(count)
		;
	return dptr->vk_device().createDescriptorPool(vk::DescriptorPoolCreateInfo()
			.setPoolSizeCount(1)
			.setPPoolSizes(&pool_size)
			.setMaxSets(1)
		);
}

static vk::DescriptorSet create_descriptor_array(DevicePtr dptr, vk::DescriptorPool pool, vk::DescriptorSetLayout layout) {
	return dptr->vk_device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(pool)
			.setDescriptorSetCount(1)
			.setPSetLayouts(&layout)
		).front();
}

static u32 max_elements(DevicePtr dptr, vk::DescriptorType type) {
	auto limits = dptr->vk_limits();
	switch(type) {
		case vk::DescriptorType::eUniformBuffer:
			return limits.maxPerStageDescriptorUniformBuffers;
		case vk::DescriptorType::eUniformBufferDynamic:
			return std::min(limits.maxDescriptorSetUniformBuffersDynamic,
							limits.maxPerStageDescriptorUniformBuffers);

		case vk::DescriptorType::eStorageBuffer:
			return limits.maxPerStageDescriptorStorageBuffers;
		case vk::DescriptorType::eStorageBufferDynamic:
			return std::min(limits.maxDescriptorSetStorageBuffersDynamic,
							limits.maxPerStageDescriptorStorageBuffers);

		case vk::DescriptorType::eCombinedImageSampler:
		case vk::DescriptorType::eSampledImage:
		case vk::DescriptorType::eUniformTexelBuffer:
				return limits.maxPerStageDescriptorSampledImages;

		case vk::DescriptorType::eStorageTexelBuffer:
		case vk::DescriptorType::eStorageImage:
				return limits.maxPerStageDescriptorStorageImages;

		default:
		break;
	};
	return fatal("Invalid binding type.");
}


DescriptorArray::DescriptorArray(DevicePtr dptr, vk::DescriptorType type) : DescriptorSetBase(dptr), _type(type) {
	vk::DescriptorSetLayoutBinding binding = vk::DescriptorSetLayoutBinding()
				.setBinding(0)
				.setDescriptorCount(max_elements(device(), _type))
				.setDescriptorType(_type)
			;

	auto layout = dptr->create_descriptor_set_layout(core::Vector({binding}));
	_pool = create_descriptor_array_pool(dptr, binding.descriptorCount, _type);
	_set = create_descriptor_array(device(), _pool, layout);
}

DescriptorArray::~DescriptorArray() {
	destroy(_pool);
}

DescriptorArray::DescriptorArray(DescriptorArray&& other) {
	swap(other);
}

DescriptorArray& DescriptorArray::operator=(DescriptorArray&& other) {
	swap(other);
	return *this;
}

void DescriptorArray::swap(DescriptorArray& other) {
	DescriptorSetBase::swap(other);
	std::swap(_type, other._type);
	std::swap(_pool, other._pool);
}

}
