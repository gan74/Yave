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
#ifndef YAVE_DESCRIPTORS_DESCRIPTORLAYOUT_H
#define YAVE_DESCRIPTORS_DESCRIPTORLAYOUT_H

#include <yave/yave.h>
#include <yave/shaders/ShaderResource.h>
#include <yave/DeviceLinked.h>

namespace yave {

class DescriptorLayout : NonCopyable, public DeviceLinked {
	public:
		DescriptorLayout() = default;
		DescriptorLayout(DevicePtr dptr, const core::Vector<ShaderStageResource>& resources);

		~DescriptorLayout();

		DescriptorLayout(DescriptorLayout&& other);
		DescriptorLayout& operator=(DescriptorLayout&& other);

		u32 set_index() const;

	private:
		void swap(DescriptorLayout& other);

		u32 _index;
		vk::DescriptorSetLayout _layout;
};

}

#endif // YAVE_DESCRIPTORS_DESCRIPTORLAYOUT_H
