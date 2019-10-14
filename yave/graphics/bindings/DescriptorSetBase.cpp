/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#include "Binding.h"

namespace yave {

void DescriptorSetBase::create_descriptor_set(DevicePtr dptr, vk::DescriptorPool pool, vk::DescriptorSetLayout layout) {
	_set = dptr->vk_device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo()
	        .setDescriptorPool(pool)
	        .setDescriptorSetCount(1)
	        .setPSetLayouts(&layout)
	    ).front();
}

void DescriptorSetBase::update_set(DevicePtr dptr, core::Span<Binding> bindings) {
	auto writes = core::vector_with_capacity<vk::WriteDescriptorSet>(bindings.size());
	for(const auto& binding : bindings) {
		auto w = vk::WriteDescriptorSet()
		        .setDstSet(_set)
		        .setDstBinding(u32(writes.size()))
		        .setDstArrayElement(0)
		        .setDescriptorCount(1)
		        .setDescriptorType(binding.vk_descriptor_type())
		    ;

		if(binding.is_buffer()) {
			w.setPBufferInfo(&binding.descriptor_info().buffer);
		} else if(binding.is_image()) {
			w.setPImageInfo(&binding.descriptor_info().image);
		} else {
			y_fatal("Unknown descriptor type.");
		}

		writes << w;
	}
	dptr->vk_device().updateDescriptorSets(u32(writes.size()), writes.begin(), 0, nullptr);
}

}
