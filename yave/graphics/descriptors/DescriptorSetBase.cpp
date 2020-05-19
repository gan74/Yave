/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include "DescriptorSetBase.h"
#include "Descriptor.h"

namespace yave {

void DescriptorSetBase::update_set(DevicePtr dptr, core::Span<Descriptor> bindings) {
	y_profile();

	auto writes = core::vector_with_capacity<VkWriteDescriptorSet>(bindings.size());
	for(const auto& binding : bindings) {
		const u32 descriptor_count = binding.descriptor_set_layout_binding(0).descriptorCount;
		VkWriteDescriptorSet write = vk_struct();
		{
			write.dstSet = _set;
			write.dstBinding = writes.size();
			write.dstArrayElement = 0;
			write.descriptorCount = descriptor_count;
			write.descriptorType = binding.vk_descriptor_type();
		}

		VkWriteDescriptorSetInlineUniformBlockEXT inline_block = vk_struct();

		if(binding.is_buffer()) {
			write.pBufferInfo = &binding.descriptor_info().buffer;
		} else if(binding.is_image()) {
			write.pImageInfo = &binding.descriptor_info().image;
		} else if(binding.is_inline_block()) {
			inline_block.pData = binding.descriptor_info().inline_block.data;
			inline_block.dataSize = binding.descriptor_info().inline_block.size;
			write.pNext = &inline_block;
			y_always_assert(inline_block.dataSize <= dptr->device_properties().max_inline_uniform_size, "Inline uniform block exceeds max supported size");
			y_debug_assert(inline_block.dataSize % 4 == 0);
		} else {
			y_fatal("Unknown descriptor type.");
		}

		writes << write;
	}

	vkUpdateDescriptorSets(dptr->vk_device(), writes.size(), writes.data(), 0, nullptr);
}

}
