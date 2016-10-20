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
#ifndef YAVE_DESCRIPTORSETLAYOUTPOOL_H
#define YAVE_DESCRIPTORSETLAYOUTPOOL_H

#include "yave.h"
#include "DeviceLinked.h"
#include <y/core/AssocVector.h>
#include <y/concurrent/Arc.h>


namespace yave {

class DescriptorSetLayoutPool : NonCopyable, public DeviceLinked {

	using LayoutKey = core::Vector<vk::DescriptorSetLayoutBinding>;

	public:
		DescriptorSetLayoutPool(DevicePtr dptr);
		~DescriptorSetLayoutPool();

		vk::DescriptorSetLayout create_descriptor_set_layout(const LayoutKey& bindings);

		vk::DescriptorSetLayout operator()(const LayoutKey& bindings) {
			return create_descriptor_set_layout(bindings);
		}

	private:
		core::AssocVector<LayoutKey, vk::DescriptorSetLayout> _layouts;


};



}

#endif // YAVE_DESCRIPTORSETLAYOUTPOOL_H
