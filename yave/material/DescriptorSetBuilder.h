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
#ifndef YAVE_DESCRIPTORSETBUILDER_H
#define YAVE_DESCRIPTORSETBUILDER_H

#include <yave/yave.h>

#include <yave/image/Sampler.h>
#include "Material.h"

namespace yave {

struct DescriptorSet {
	vk::DescriptorPool pool;
	vk::DescriptorSet set;
	vk::DescriptorSetLayout layout;
	core::Vector<UniformBinding> ub_binding;
	core::Vector<TextureBinding> tx_binding;

	void destroy(DevicePtr device);
};

class DescriptorSetBuilder : public DeviceLinked {
	public:
		DescriptorSetBuilder(DevicePtr dptr);

		vk::DescriptorPool create_pool(usize set_count) const;

		DescriptorSet build(const Material& material) const;

	private:
		Sampler _default_sampler;
};
}

#endif // YAVE_DESCRIPTORSETBUILDER_H
