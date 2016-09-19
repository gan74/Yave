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
#ifndef YAVE_MATERIAL_MATERIALDESCRIPTORSET_H
#define YAVE_MATERIAL_MATERIALDESCRIPTORSET_H

#include <yave/yave.h>

namespace yave {

struct MaterialDescriptorSet {
	public:
		MaterialDescriptorSet();
		MaterialDescriptorSet(vk::DescriptorPool pool, vk::DescriptorSet set, vk::DescriptorSetLayout layout);

		const vk::DescriptorSet& get_vk_descriptor_set() const;
		const vk::DescriptorSetLayout& get_vk_layout() const;

		void destroy(DevicePtr device);

	private:
		vk::DescriptorPool _pool;
		vk::DescriptorSet _set;
		vk::DescriptorSetLayout _layout;

};


}

#endif // YAVE_MATERIAL_MATERIALDESCRIPTORSET_H
