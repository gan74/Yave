/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
#ifndef YAVE_GRAPHICS_BINDINGS_DESCRIPTORARRAY_H
#define YAVE_GRAPHICS_BINDINGS_DESCRIPTORARRAY_H

#include "DescriptorSetBase.h"

namespace yave {

class DescriptorArray : public DescriptorSetBase {

	public:
		DescriptorArray() = default;
		DescriptorArray(DevicePtr dptr, vk::DescriptorType type, usize max_size = -1);

		~DescriptorArray();

		DescriptorArray(DescriptorArray&& other);
		DescriptorArray& operator=(DescriptorArray&& other);

		usize capacity() const;

		void set(usize index, const Binding& binding);


		auto operator[](usize i) {
			class Ref {
				public:
					void operator=(const Binding& b) {
						_array.set(_index, b);
					}

					Ref(DescriptorArray& a, usize i) : _array(a), _index(i) {
					}

				private:
					DescriptorArray& _array;
					usize _index;
			};

			return Ref(*this, i);
		}

	protected:
		void swap(DescriptorArray& other);

		vk::DescriptorType _type;
		vk::DescriptorPool _pool;
		usize _max_elements;
};

}

#endif // YAVE_GRAPHICS_BINDINGS_DESCRIPTORARRAY_H
