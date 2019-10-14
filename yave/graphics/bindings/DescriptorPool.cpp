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

#include "DescriptorPool.h"

#include <yave/graphics/bindings/Binding.h>

namespace yave {

DescriptorPoolSize::DescriptorPoolSize(core::Span<Binding> bindings) {
	for(const auto& binding : bindings) {
		++_sizes[usize(binding.vk_descriptor_type())];
	}
}

const math::Vec<DescriptorPoolSize::max_descriptor_type, u32>& DescriptorPoolSize::sizes() const {
	return _sizes;
}

u32 DescriptorPoolSize::operator[](vk::DescriptorType i) const {
	y_debug_assert(usize(i) < max_descriptor_type);
	return _sizes[usize(i)];
}

DescriptorPoolSize& DescriptorPoolSize::operator+=(const DescriptorPoolSize& other) {
	_sizes += other._sizes;
	return *this;
}

DescriptorPoolSize DescriptorPoolSize::operator+(const DescriptorPoolSize& other) const {
	DescriptorPoolSize s = *this;
	s += other;
	return s;
}


static vk::DescriptorPool create_descriptor_pool(DevicePtr dptr, const DescriptorPoolSize& size) {
	usize sizes_count = 0;
	std::array<vk::DescriptorPoolSize, DescriptorPoolSize::max_descriptor_type> sizes;

	for(usize i = 0; i != DescriptorPoolSize::max_descriptor_type; ++i) {
		vk::DescriptorType type = vk::DescriptorType(i);
		if(size[type]) {
			sizes[sizes_count++]
					.setType(vk::DescriptorType(i))
					.setDescriptorCount(size[type])
				;
		}
	}

	if(!sizes_count) {
		return {};
	}

	return dptr->vk_device().createDescriptorPool(vk::DescriptorPoolCreateInfo()
			.setPoolSizeCount(sizes_count)
			.setPPoolSizes(sizes.begin())
			.setMaxSets(1)
		);
}


DescriptorPool::DescriptorPool(DevicePtr dptr, core::Span<Binding> bindings) : DescriptorPool(dptr, DescriptorPoolSize(bindings)) {
}

DescriptorPool::DescriptorPool(DevicePtr dptr, const DescriptorPoolSize& size) :
		DeviceLinked(dptr),
		_pool(create_descriptor_pool(dptr, size)) {

}

DescriptorPool::~DescriptorPool() {
	destroy(_pool);
}

vk::DescriptorPool DescriptorPool::vk_pool() const {
	return _pool;
}

}
