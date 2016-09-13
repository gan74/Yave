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
#ifndef YAVE_UNIFORMBINDING_H
#define YAVE_UNIFORMBINDING_H

#include "yave.h"

#include <yave/buffer/BufferReference.h>

namespace yave {

class UniformBinding {

	public:
		UniformBinding(u32 binding, const BufferReference<BufferUsage::UniformBuffer>& buffer) : _buffer(buffer), _binding(binding) {
		}

		auto get_descriptor_buffer_info() const {
			return _buffer.descriptor_info();
		}

		u32 get_binding() const {
			return _binding;
		}

		auto descriptor_set_layout_binding() const {
			return vk::DescriptorSetLayoutBinding()
					.setBinding(_binding)
					.setDescriptorCount(1)
					.setDescriptorType(vk::DescriptorType::eUniformBuffer)
					.setStageFlags(vk::ShaderStageFlagBits::eAll)
				;
		}

	private:
		BufferReference<BufferUsage::UniformBuffer> _buffer;
		u32 _binding;
};

}


#endif // YAVE_UNIFORMBINDING_H
