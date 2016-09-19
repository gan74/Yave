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
#include "Material.h"
#include "MaterialCompiler.h"
#include "MaterialData.h"

#include <yave/Device.h>

namespace yave {

vk::DescriptorPool create_descriptor_pool(DevicePtr dptr, const MaterialData& data) {
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
			.setMaxSets(data._ub_bindings.size() + data._tx_bindings.size())
		);
}

vk::DescriptorSetLayout create_descriptor_layout(DevicePtr dptr, const MaterialData& data) {
	auto bindings = core::Vector<vk::DescriptorSetLayoutBinding>();

	bindings << core::range(data._ub_bindings).map([](const UniformBinding& binding) {
			return binding.descriptor_set_layout_binding();
		});

	bindings << core::range(data._tx_bindings).map([](const TextureBinding& binding) {
			return binding.descriptor_set_layout_binding();
		});

	return dptr->get_vk_device().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(bindings.size())
				.setPBindings(bindings.begin())
			);
}

void update_texture_sets(DevicePtr dptr, vk::DescriptorSet set, const Sampler &sampler, const MaterialData& data) {
	auto infos = core::range(data._tx_bindings).map([&](const TextureBinding& binding) {
			return vk::DescriptorImageInfo()
					.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
					.setImageView(binding.get_image_view().get_vk_image_view())
					.setSampler(sampler.get_vk_sampler())
				;
		}).collect<core::Vector>();

	auto writes = core::Vector<vk::WriteDescriptorSet>();
	for(usize i = 0; i != data._tx_bindings.size(); i++) {
		const auto& binding = data._tx_bindings[i];
		writes << vk::WriteDescriptorSet()
				.setDstSet(set)
				.setDstBinding(binding.get_binding())
				.setDstArrayElement(0)
				.setDescriptorCount(1)
				.setPImageInfo(infos.begin() + i)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			;
	}
	dptr->get_vk_device().updateDescriptorSets(writes.size(), writes.begin(), 0, nullptr);
}

void update_buffer_sets(DevicePtr dptr, vk::DescriptorSet set, const MaterialData& data) {
	auto infos = core::range(data._ub_bindings).map([&](const UniformBinding& binding) {
			return binding.get_descriptor_buffer_info();
		}).collect<core::Vector>();

	auto writes = core::Vector<vk::WriteDescriptorSet>();
	for(usize i = 0; i != data._ub_bindings.size(); i++) {
		const auto& binding = data._ub_bindings[i];
		writes << vk::WriteDescriptorSet()
				.setDstSet(set)
				.setDstBinding(binding.get_binding())
				.setDstArrayElement(0)
				.setDescriptorCount(1)
				.setPBufferInfo(infos.begin() + i)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			;
	}
	dptr->get_vk_device().updateDescriptorSets(writes.size(), writes.begin(), 0, nullptr);
}

vk::DescriptorSet create_descriptor_set(DevicePtr dptr, vk::DescriptorPool pool, vk::DescriptorSetLayout layout) {
	return dptr->get_vk_device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(pool)
			.setDescriptorSetCount(1)
			.setPSetLayouts(&layout)
		).front();
}





Material::Material(DevicePtr dptr, const MaterialData &data) :
		DeviceLinked(dptr),
		_data(data),
		_sampler(dptr),
		_pool(create_descriptor_pool(dptr, data)),
		_layout(create_descriptor_layout(dptr, data)),
		_set(create_descriptor_set(dptr, _pool, _layout)) {

	update_texture_sets(dptr, _set, _sampler, _data);
	update_buffer_sets(dptr, _set, _data);
}

Material::~Material() {
	_compiled.clear();
	destroy(_layout);
	destroy(_pool);
}

Material::Material(Material&& other) {
	swap(other);
}

Material& Material::operator=(Material&& other) {
	swap(other);
	return *this;
}

void Material::swap(Material& other) {
	DeviceLinked::swap(other);
	std::swap(_data, other._data);
	std::swap(_pool, other._pool);
	std::swap(_layout, other._layout);
	std::swap(_set, other._set);
	std::swap(_compiled, other._compiled);
}

const GraphicPipeline& Material::compile(const RenderPass& render_pass, const Viewport& viewport) {
	auto key = render_pass.get_vk_render_pass();
	auto it = core::range(_compiled).find(key);
	if(it == _compiled.end()) {
		MaterialCompiler compiler(get_device());
		_compiled.insert(key, compiler.compile(*this, render_pass, viewport));
		return _compiled.last().second;
	}
	return it->second;
}

const MaterialData& Material::get_data() const {
	return _data;
}

const vk::DescriptorSet& Material::get_vk_descriptor_set() const {
	return _set;
}

const vk::DescriptorSetLayout& Material::get_vk_descriptor_set_layout() const {
	return _layout;
}

}
