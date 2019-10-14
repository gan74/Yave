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
#ifndef YAVE_GRAPHICS_DESCRIPTORPOOL_H
#define YAVE_GRAPHICS_DESCRIPTORPOOL_H

#include "DescriptorSetBase.h"

#include <yave/device/DeviceLinked.h>

namespace yave {

class DescriptorPoolSize {
	public:
		static constexpr usize max_descriptor_type = std::max({
		        usize(vk::DescriptorType::eSampler),
		        usize(vk::DescriptorType::eCombinedImageSampler),
		        usize(vk::DescriptorType::eSampledImage),
		        usize(vk::DescriptorType::eStorageImage),
		        usize(vk::DescriptorType::eUniformTexelBuffer),
		        usize(vk::DescriptorType::eStorageTexelBuffer),
		        usize(vk::DescriptorType::eUniformBuffer),
		        usize(vk::DescriptorType::eStorageBuffer),
		        usize(vk::DescriptorType::eUniformBufferDynamic),
		        usize(vk::DescriptorType::eStorageBufferDynamic)
		    }) + 1;


		DescriptorPoolSize() = default;
		DescriptorPoolSize(core::Span<Descriptor> bindings);

		void add_descriptor(const Descriptor& d);

		const math::Vec<max_descriptor_type, u32>& sizes() const;

		u32 operator[](vk::DescriptorType i) const;

		DescriptorPoolSize& operator+=(const DescriptorPoolSize& other);
		DescriptorPoolSize operator+(const DescriptorPoolSize& other) const;

	private:
		math::Vec<max_descriptor_type, u32> _sizes;
};

class DescriptorPool : public DeviceLinked, NonCopyable {
	public:
		DescriptorPool() = default;
		DescriptorPool(DescriptorPool&&) = default;
		DescriptorPool& operator=(DescriptorPool&&) = default;

		DescriptorPool(DevicePtr dptr, core::Span<Descriptor> bindings);
		DescriptorPool(DevicePtr dptr, const DescriptorPoolSize& size);

		~DescriptorPool();

		vk::DescriptorPool vk_pool() const;

	protected:
		vk::DescriptorPool _pool;
};

}

#endif // YAVE_GRAPHICS_DESCRIPTORPOOL_H
