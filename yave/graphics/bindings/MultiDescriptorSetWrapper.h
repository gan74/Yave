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
#ifndef YAVE_GRAPHICS_BINDINGS_MULTIDESCRIPTORSETWRAPPER_H
#define YAVE_GRAPHICS_BINDINGS_MULTIDESCRIPTORSETWRAPPER_H

#include "DescriptorSet.h"

#include <yave/graphics/swapchain/FrameToken.h>

namespace yave {
/*
 *
 * WARNING: modifying one descriptor set will not modify the others.
 *
 */

/*template<typename T = DescriptorSet>
class MultiDescriptorSetWrapper {
	public:
		MultiDescriptorSetWrapper(const core::ArrayView<Binding>& bindings) : _bindings(bindings) {
		}

		T& operator[](const FrameToken& token) {
			lazy_init(token);
			return _sets[token.image_index];
		}

	private:
		void lazy_init(const FrameToken& token) {
			if(is_initialized()) {
				return;
			}
			DevicePtr dptr = token.image_view.device();
			_sets = std::make_unique<DescriptorSet[]>(token.image_count);
			for(usize i = 0; i != token.image_count; ++i) {
				_sets[i] = T(dptr, _bindings);
			}
			_bindings.clear();
		}

		bool is_initialized() const {
			return _sets.get();
		}

		std::unique_ptr<T[]> _sets;
		core::Vector<Binding> _bindings;
};*/

}

#endif // YAVE_GRAPHICS_BINDINGS_MULTIDESCRIPTORSETWRAPPER_H
