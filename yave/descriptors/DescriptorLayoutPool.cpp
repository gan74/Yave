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

#include "DescriptorLayoutPool.h"

namespace yave {

DescriptorLayoutPool::DescriptorLayoutPool(DevicePtr dptr) : DeviceLinked(dptr) {
}

const DescriptorLayout& DescriptorLayoutPool::operator[](const core::Vector<ShaderStageResource>& res) {
	auto& layout = _layouts[res];
	if(!layout) {
		layout = new DescriptorLayout(get_device(), res);
	}
	return *layout;
}

DescriptorLayoutPool::DescriptorLayoutPool(DescriptorLayoutPool&& other) {
	swap(other);
}

DescriptorLayoutPool& DescriptorLayoutPool::operator=(DescriptorLayoutPool&& other) {
	swap(other);
	return *this;
}

void DescriptorLayoutPool::swap(DescriptorLayoutPool& other) {
	DeviceLinked::swap(other);

	std::swap(_layouts, other._layouts);
}

}
