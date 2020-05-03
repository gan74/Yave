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

	auto writes = core::vector_with_capacity<vk::WriteDescriptorSet>(bindings.size());
	for(const auto& binding : bindings) {
		const u32 descriptor_count = binding.descriptor_set_layout_binding(0).descriptorCount;
		auto w = vk::WriteDescriptorSet()
				.setDstSet(_set)
				.setDstBinding(u32(writes.size()))
				.setDstArrayElement(0)
				.setDescriptorCount(descriptor_count)
				.setDescriptorType(binding.vk_descriptor_type())
			;

		vk::WriteDescriptorSetInlineUniformBlockEXT inline_block;
		if(binding.is_buffer()) {
			w.setPBufferInfo(&binding.descriptor_info().buffer);
		} else if(binding.is_image()) {
			w.setPImageInfo(&binding.descriptor_info().image);
		} else if(binding.is_inline_block()) {
			inline_block.setPData(binding.descriptor_info().inline_block.data);
			inline_block.setDataSize(binding.descriptor_info().inline_block.size);
			w.setPNext(&inline_block);
			y_always_assert(inline_block.dataSize <= dptr->device_properties().max_inline_uniform_size, "Inline uniform block exceeds max supported size");
			y_debug_assert(inline_block.dataSize % 4 == 0);
		} else {
			y_fatal("Unknown descriptor type.");
		}

		writes << w;
	}
	dptr->vk_device().updateDescriptorSets(u32(writes.size()), writes.begin(), 0, nullptr);
}

}
