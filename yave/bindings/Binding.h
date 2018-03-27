/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef YAVE_BINDINGS_BINDING_H
#define YAVE_BINDINGS_BINDING_H

#include <yave/yave.h>
#include <yave/device/Device.h>

#include <yave/images/ImageView.h>
#include <yave/buffers/TypedSubBuffer.h>

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

		template<ImageType Type>
		Binding(const ImageView<ImageUsage::TextureBit, Type>& view) :
				 _type(vk::DescriptorType::eCombinedImageSampler),
				 _info(vk::DescriptorImageInfo()
					.setImageLayout(vk_image_layout(view.usage()))
					.setImageView(view.vk_view())
					.setSampler(view.device()->vk_sampler())) {
		}

		template<ImageType Type>
		Binding(const ImageView<ImageUsage::StorageBit, Type>& view) :
				 _type(vk::DescriptorType::eStorageImage),
				 _info(vk::DescriptorImageInfo()
					.setImageLayout(vk_image_layout(ImageUsage::StorageBit))
					.setImageView(view.vk_view())
					.setSampler(view.device()->vk_sampler())) {
		}

		template<ImageUsage Usage, ImageType Type>
		Binding(const Image<Usage, Type>& image) : Binding(ImageView<Usage, Type>(image)) {
		}


		Binding(const SubBuffer<BufferUsage::UniformBit>& buffer) :
				_type(vk::DescriptorType::eUniformBuffer),
				_info(buffer.descriptor_info()) {
		}

		Binding(const SubBuffer<BufferUsage::StorageBit>& buffer) :
				_type(vk::DescriptorType::eStorageBuffer),
				_info(buffer.descriptor_info()) {
		}

		template<BufferUsage Usage, MemoryType Memory>
		Binding(const Buffer<Usage, Memory>& buffer) :
				Binding(SubBuffer<(Usage & BufferUsage::StorageBit) != BufferUsage::None ? BufferUsage::StorageBit : BufferUsage::UniformBit>(buffer)) {
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

		bool is_buffer() const {
			switch(_type) {
				case vk::DescriptorType::eUniformTexelBuffer:
				case vk::DescriptorType::eStorageTexelBuffer:
				case vk::DescriptorType::eUniformBuffer:
				case vk::DescriptorType::eStorageBuffer:
				case vk::DescriptorType::eUniformBufferDynamic:
				case vk::DescriptorType::eStorageBufferDynamic:
					return true;
				default:
					break;
			};
			return false;
		}

		bool is_image() const {
			switch(_type) {
				case vk::DescriptorType::eSampler:
				case vk::DescriptorType::eCombinedImageSampler:
				case vk::DescriptorType::eSampledImage:
				case vk::DescriptorType::eStorageImage:
					return true;
				default:
					break;
			};
			return false;
		}

	private:
		vk::DescriptorType _type;
		DescriptorInfo _info;

};

}

#endif // YAVE_BINDINGS_BINDING_H
