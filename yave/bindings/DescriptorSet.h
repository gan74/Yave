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
#ifndef YAVE_BINDINGS_DESCRIPTORSET_H
#define YAVE_BINDINGS_DESCRIPTORSET_H

#include "Binding.h"

namespace yave {

class DescriptorSet : NonCopyable, public DeviceLinked {

	public:
		DescriptorSet() = default;
		DescriptorSet(DevicePtr dptr, const core::ArrayView<Binding>& bindings);

		~DescriptorSet();

		DescriptorSet(DescriptorSet&& other);
		DescriptorSet& operator=(DescriptorSet&& other);

		const vk::DescriptorSet& vk_descriptor_set() const;

	private:
		void swap(DescriptorSet& other);

		vk::DescriptorPool _pool;
		vk::DescriptorSet _set;
};

}

#endif // YAVE_BINDINGS_DESCRIPTORSET_H
