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

#include "DescriptorSetLayoutPool.h"
#include "Device.h"


namespace yave {

DescriptorSetLayoutPool::DescriptorSetLayoutPool(DevicePtr dptr) : DeviceLinked(dptr) {
}

DescriptorSetLayoutPool::~DescriptorSetLayoutPool() {
	for(const auto& dsl : _layouts) {
		destroy(dsl.second);
	}
}

vk::DescriptorSetLayout DescriptorSetLayoutPool::create_descriptor_set_layout(const LayoutKey& bindings) {
	auto& layout = _layouts[bindings];
	if(!layout) {
		layout = device()->vk_device().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo()
				.setBindingCount(u32(bindings.size()))
				.setPBindings(bindings.begin())
			);
	}
	return layout;
}



}
