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

#include "DescriptorLayout.h"
#include <yave/Device.h>

namespace yave {

static vk::DescriptorSetLayout create_descriptor_layout(DevicePtr dptr, const core::Vector<vk::DescriptorSetLayoutBinding>& bindings) {
	return dptr->get_vk_device().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(bindings.size())
				.setPBindings(bindings.begin())
			);
}

static u32 compute_index(const core::Vector<ShaderStageResource>& resources) {
	u32 index = 0;
	if(!resources.is_empty()) {
		index = resources.first().resource.set;
		for(const auto& r : resources) {
			if(r.resource.set != index) {
				fatal("Invaid descriptor set index");
			}
		}
	}
	return index;
}

static core::Vector<vk::DescriptorSetLayoutBinding> create_layout_bindings(const core::Vector<ShaderStageResource>& resources) {
	return core::range(resources).map([](ShaderStageResource res) {
			return vk::DescriptorSetLayoutBinding()
					.setBinding(res.resource.binding)
					.setDescriptorCount(1)
					.setDescriptorType(vk::DescriptorType(res.resource.type))
					.setStageFlags(res.stage)
				;
		}).collect<core::Vector>();
}


DescriptorLayout::DescriptorLayout(DevicePtr dptr, const core::Vector<ShaderStageResource>& resources) :
		DeviceLinked(dptr),
		_index(compute_index(resources)),
		_layout(create_descriptor_layout(dptr, create_layout_bindings(resources))) {
}

DescriptorLayout::~DescriptorLayout() {
	destroy(_layout);
}

DescriptorLayout::DescriptorLayout(DescriptorLayout&& other) {
	swap(other);
}

DescriptorLayout& DescriptorLayout::operator=(DescriptorLayout&& other) {
	swap(other);
	return *this;
}

void DescriptorLayout::swap(DescriptorLayout& other) {
	DeviceLinked::swap(other);
	std::swap(_index, other._index);
	std::swap(_layout, other._layout);
}

u32 DescriptorLayout::set_index() const {
	return _index;
}

}
