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
#include "DescriptorSetBuilder.h"

#include <yave/LowLevelGraphics.h>

namespace yave {


void DescriptorSet::destroy(DevicePtr dptr) {
	if(dptr) {
		auto vk = dptr->get_vk_device();
		vk.destroyDescriptorSetLayout(layout);
		vk.destroyDescriptorPool(pool);
	}
}

DescriptorSetBuilder::DescriptorSetBuilder(DevicePtr dptr) : DeviceLinked(dptr), _default_sampler(dptr) {
}

vk::DescriptorPool DescriptorSetBuilder::create_pool(usize set_count) const {
	auto ub_pool_size = vk::DescriptorPoolSize()
			.setDescriptorCount(1)
			.setType(vk::DescriptorType::eUniformBuffer)
		;

	auto tx_pool_size = vk::DescriptorPoolSize()
			.setDescriptorCount(1)
			.setType(vk::DescriptorType::eCombinedImageSampler)
		;

	vk::DescriptorPoolSize sizes[] = {ub_pool_size, tx_pool_size};
	return get_device()->get_vk_device().createDescriptorPool(vk::DescriptorPoolCreateInfo()
			.setPoolSizeCount(2)
			.setPPoolSizes(sizes)
			.setMaxSets(set_count)
		);
}

DescriptorSet DescriptorSetBuilder::build(const Material& material) const {
	if(!material.binding_count()) {
		return {};
	}

	usize set_count = material.binding_count();

	auto pool = create_pool(set_count);

	auto mat_buffer_bindings = core::range(material.get_uniform_bindings());
	auto mat_texture_bindings = core::range(material.get_texture_bindings());

	auto bindings = core::Vector<vk::DescriptorSetLayoutBinding>();

	bindings << mat_buffer_bindings.map([](const UniformBinding& binding) {
			return binding.descriptor_set_layout_binding();
		});

	bindings << mat_texture_bindings.map([](const TextureBinding& binding) {
			return binding.descriptor_set_layout_binding();
		});

	auto layout = get_device()->get_vk_device().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(bindings.size())
				.setPBindings(bindings.begin())
			);

	auto set = get_device()->get_vk_device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(pool)
			.setDescriptorSetCount(1)
			.setPSetLayouts(&layout)
		).front();

	auto buffer_infos = mat_buffer_bindings.map([](const UniformBinding& binding) {
			return binding.get_descriptor_buffer_info();
		}).collect<core::Vector>();

	auto image_infos = mat_texture_bindings.map([&](const TextureBinding& binding) {
		return vk::DescriptorImageInfo()
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setImageView(binding.get_image_view().get_vk_image_view())
				.setSampler(_default_sampler.get_vk_sampler())
			;
	}).collect<core::Vector>();

	auto writes = core::Vector<vk::WriteDescriptorSet>();
	for(usize i = 0; i != material.get_uniform_bindings().size(); i++) {
		const auto& binding = material.get_uniform_bindings()[i];
		writes << vk::WriteDescriptorSet()
				.setDstSet(set)
				.setDstBinding(binding.get_binding())
				.setDstArrayElement(0)
				.setDescriptorCount(1)
				.setPBufferInfo(buffer_infos.begin() + i)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			;
	}

	for(usize i = 0; i != material.get_texture_bindings().size(); i++) {
		const auto& binding = material.get_texture_bindings()[i];
		writes << vk::WriteDescriptorSet()
				.setDstSet(set)
				.setDstBinding(binding.get_binding())
				.setDstArrayElement(0)
				.setDescriptorCount(1)
				.setPImageInfo(image_infos.begin() + i)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			;
	};

	get_device()->get_vk_device().updateDescriptorSets(writes.size(), writes.begin(), 0, nullptr);

	return DescriptorSet{
			pool,
			set,
			layout,
			material.get_uniform_bindings(),
			material.get_texture_bindings()
		};
}

}
