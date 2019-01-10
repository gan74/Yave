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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPHDESCRIPTORBINDING_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPHDESCRIPTORBINDING_H

#include "FrameGraphResourceToken.h"

#include <yave/graphics/bindings/Binding.h>

namespace yave {

class FrameGraphDescriptorBinding {
	enum class BindingType {
		None,

		InputImage,
		StorageImage,

		InputBuffer,
		StorageBuffer,

		External
	};

	public:
		FrameGraphDescriptorBinding(const Binding& bind);

		static FrameGraphDescriptorBinding create_storage_binding(FrameGraphBufferId res);
		static FrameGraphDescriptorBinding create_storage_binding(FrameGraphImageId res);

		static FrameGraphDescriptorBinding create_uniform_binding(FrameGraphBufferId res);
		static FrameGraphDescriptorBinding create_uniform_binding(FrameGraphImageId res);

		Binding create_binding(FrameGraphResourcePool* pool) const;

	private:
		FrameGraphDescriptorBinding(FrameGraphBufferId res, BindingType type);
		FrameGraphDescriptorBinding(FrameGraphImageId res, BindingType type);

		BindingType _type = BindingType::None;
		union {
			FrameGraphImageId _image;
			FrameGraphBufferId _buffer;
			Binding _external;
		};
};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHDESCRIPTORBINDING_H