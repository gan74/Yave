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
#ifndef YAVE_DESCRIPTORS_DESCRIPTORSET_H
#define YAVE_DESCRIPTORS_DESCRIPTORSET_H

#include "Binding.h"

namespace yave {

class DescriptorSet : NonCopyable, public DeviceLinked {

	public:
		DescriptorSet(DevicePtr dptr, std::initializer_list<Binding> bindings);

	private:
		vk::DescriptorPool _pool;
		vk::DescriptorSetLayout _layout;
		vk::DescriptorSet _set;
};

}

#endif // YAVE_DESCRIPTORS_DESCRIPTORSET_H
