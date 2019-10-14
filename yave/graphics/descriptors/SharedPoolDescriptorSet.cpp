/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

#include "SharedPoolDescriptorSet.h"
#include "Descriptor.h"

namespace yave {

SharedPoolDescriptorSet::SharedPoolDescriptorSet(DescriptorPool& pool, core::Span<Descriptor> bindings) {
	if(!bindings.is_empty()) {
		auto layout_bindings = core::vector_with_capacity<vk::DescriptorSetLayoutBinding>(bindings.size());

		for(const auto& binding : bindings) {
			layout_bindings << binding.descriptor_set_layout_binding(layout_bindings.size());
		}

		DevicePtr dptr = pool.device();
		auto layout = dptr->create_descriptor_set_layout(layout_bindings);

		create_descriptor_set(dptr, pool.vk_pool(), layout);
		update_set(dptr, bindings);
	}
}

}
