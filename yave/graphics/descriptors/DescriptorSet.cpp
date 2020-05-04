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

#include "DescriptorSet.h"
#include "Descriptor.h"

namespace yave {

static core::Vector<VkDescriptorSetLayoutBinding> create_layout_bindings(core::Span<Descriptor> bindings) {
	auto layout_bindings = core::vector_with_capacity<VkDescriptorSetLayoutBinding>(bindings.size());
	for(const Descriptor& b : bindings) {
		layout_bindings << b.descriptor_set_layout_binding(layout_bindings.size());
	}
	return layout_bindings;
}


DescriptorSet::DescriptorSet(DevicePtr dptr, core::Span<Descriptor> bindings) {
	if(!bindings.is_empty()) {
		_data = dptr->descriptor_set_allocator().create_descritptor_set(create_layout_bindings(bindings));
		_set = _data.vk_descriptor_set();

		update_set(dptr, bindings);
	}
}

DescriptorSet::~DescriptorSet() {
	if(const DevicePtr dptr = device()) {
		dptr->destroy(std::move(_data));
	}
}

DescriptorSet::DescriptorSet(DescriptorSet&& other) {
	swap(other);
}

DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other) {
	swap(other);
	return *this;
}

DevicePtr DescriptorSet::device() const {
	return _data.device();
}

bool DescriptorSet::is_null() const  {
	return _data.is_null();
}

VkDescriptorSetLayout DescriptorSet::vk_descriptor_set_layout() const {
	return _data.vk_descriptor_set_layout();
}

void DescriptorSet::swap(DescriptorSet& other) {
	std::swap(_data, other._data);
	std::swap(_set, other._set);
}


}
