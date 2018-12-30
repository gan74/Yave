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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPHRECOURCE_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPHRECOURCE_H

#include <yave/yave.h>
#include <yave/graphics/barriers/PipelineStage.h>

#include "TransientImage.h"
#include "TransientBuffer.h"

#include <typeindex>

namespace yave {

class FrameGraph;
class FrameGraphPass;
class FrameGraphPassBuilder;
class FrameGraphResourcePool;

class FrameGraphResourceBase {
	public:
		FrameGraphResourceBase() = default;

		u32 id() const {
			return u32(_id);
		}

		bool operator==(const FrameGraphResourceBase& other) const {
			return _id == other._id;
		}

		bool is_valid() const {
			return _id != invalid_id;
		}

	protected:
		friend class FrameGraph;

		static constexpr u32 invalid_id = u32(-1);

		u32 _id = invalid_id;
};

template<typename T>
class FrameGraphResource : public FrameGraphResourceBase {
	public:
		using resource_type = T;

		FrameGraphResource() = default;
};

using FrameGraphImage = FrameGraphResource<TransientImage<>>;
using FrameGraphBuffer = FrameGraphResource<TransientBuffer>;

template<typename T>
struct FrameGraphTypedBuffer : FrameGraphBuffer {
	using FrameGraphBuffer::FrameGraphBuffer;

	FrameGraphTypedBuffer(const FrameGraphBuffer& other) : FrameGraphBuffer(other) {
	}
};

}


namespace std {
template<>
struct hash<yave::FrameGraphResourceBase> : hash<y::u32>{
	auto operator()(yave::FrameGraphResourceBase r) const {
		return hash<y::u32>::operator()(r.id());
	}
};

template<typename T>
struct hash<yave::FrameGraphResource<T>> : hash<yave::FrameGraphResourceBase> {
};
}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHRECOURCE_H
