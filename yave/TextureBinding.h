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
#ifndef YAVE_TEXTUREBINDING_H
#define YAVE_TEXTUREBINDING_H

#include "yave.h"

#include <y/core/Ptr.h>
#include <yave/image/ImageView.h>

namespace yave {

class TextureBinding {

	public:
		TextureBinding(u32 binding, const TextureView& view) : _binding(binding), _view(view) {
		}

		u32 get_binding() const {
			return _binding;
		}

		const TextureView& get_image_view() const {
			return _view;
		}

		auto descriptor_set_layout_binding() const {
			return vk::DescriptorSetLayoutBinding()
					.setBinding(_binding)
					.setDescriptorCount(1)
					.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
					.setStageFlags(vk::ShaderStageFlagBits::eAll)
				;
		}


	private:
		u32 _binding;
		TextureView _view;
};

}


#endif // YAVE_TEXTUREBINDING_H
