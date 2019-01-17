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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPHRESOURCETOKEN_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPHRESOURCETOKEN_H

#include "FrameGraphResourceId.h"

namespace yave {

/*template<BufferUsage Usage>
class FrameGraphBufferToken {
	public:
		static constexpr BufferUsage usage = Usage;

		using sub_buffer_type = SubBuffer<usage>;


		bool is_valid() const {
			return _resource.is_valid();
		}

		FrameGraphBufferId resource() const {
			return _resource;
		}

		FrameGraphBufferToken() = default;

	protected:
		friend class FrameGraph;
		friend class FrameGraphResourcePool;
		friend class FrameGraphPassBuilder;

		FrameGraphBufferToken(FrameGraphBufferId res) : _resource(res) {
		}

	private:
		FrameGraphBufferId _resource;
};

template<typename T, BufferUsage Usage>
class FrameGraphTypedBufferToken : public FrameGraphBufferToken<Usage> {
	public:
		using value_type = T;
		using sub_buffer_type = TypedSubBuffer<T, Usage>;

		FrameGraphTypedBufferToken() = default;

	protected:
		FrameGraphTypedBufferToken(FrameGraphBufferId res) : FrameGraphBufferToken<Usage>(res) {
		}
};

template<typename T>
class FrameGraphBufferCpuToken : public FrameGraphTypedBufferToken<T, BufferUsage::None> {
	public:
		FrameGraphBufferCpuToken() = default;

	protected:
		friend class FrameGraph;
		friend class FrameGraphResourcePool;
		friend class FrameGraphPassBuilder;

		FrameGraphBufferCpuToken(FrameGraphBufferId res) : FrameGraphTypedBufferToken<T, BufferUsage::None>(res) {
		}
};

using FrameGraphUniformBufferToken = FrameGraphBufferToken<BufferUsage::UniformBit>;
using FrameGraphAttribBufferToken = FrameGraphBufferToken<BufferUsage::AttributeBit>;

template<typename T>
using FrameGraphTypedUniformBufferToken = FrameGraphTypedBufferToken<T, BufferUsage::UniformBit>;
template<typename T>
using FrameGraphTypedAttribBufferToken = FrameGraphTypedBufferToken<T, BufferUsage::AttributeBit>;


static_assert(std::is_copy_constructible_v<FrameGraphBufferCpuToken<int>>);
static_assert(std::is_copy_constructible_v<FrameGraphAttribBufferToken>);*/

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHRESOURCETOKEN_H
