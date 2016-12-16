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
#ifndef YAVE_DESCRIPTORS_BINDING_H
#define YAVE_DESCRIPTORS_BINDING_H

#include <yave/yave.h>
#include <yave/Device.h>

#include <yave/image/ImageView.h>
#include <yave/buffer/TypedBuffer.h>

namespace yave {

class Binding {

	public:
		union DescriptorInfo {
			vk::DescriptorImageInfo image;
			vk::DescriptorBufferInfo buffer;

			DescriptorInfo(vk::DescriptorImageInfo i) : image(i) {
			}
			DescriptorInfo(vk::DescriptorBufferInfo b) : buffer(b) {
			}
		};


		Binding(const TextureView& view) :
				 _type(vk::DescriptorType::eCombinedImageSampler),
				 _info(vk::DescriptorImageInfo()
					.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
					.setImageView(view.vk_image_view())
					.setSampler(view.image().device()->vk_sampler())) {
		}

		template<typename T, MemoryFlags Flags>
		Binding(const TypedBuffer<T, BufferUsage::UniformBuffer, Flags>& buffer) :
				_type(vk::DescriptorType::eUniformBuffer),
				_info(buffer.descriptor_info()) {
		}

		auto descriptor_set_layout_binding(usize index) const {
			return vk::DescriptorSetLayoutBinding()
					.setBinding(u32(index))
					.setDescriptorCount(1)
					.setDescriptorType(_type)
					.setStageFlags(vk::ShaderStageFlagBits::eAll)
				;
		}

		const DescriptorInfo& descriptor_info() const {
			return _info;
		}

		vk::DescriptorType vk_descriptor_type() const {
			return _type;
		}

	private:
		vk::DescriptorType _type;
		DescriptorInfo _info;

};

}

#endif // YAVE_DESCRIPTORS_BINDING_H
