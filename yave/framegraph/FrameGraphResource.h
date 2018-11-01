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

#include <typeindex>

namespace yave {

class FrameGraphPassBase;
class FrameGraphBuilder;
class FrameGraphResources;

class FrameGraphResourceBase {
	public:
		FrameGraphResourceBase() = default;

		u32 id() const {
			return u32(_id);
		}

		bool is_valid() const {
			return _id != invalid_id;
		}

		bool is_initialized() const {
			return _last_pass_to_write;
		}

		const FrameGraphPassBase* last_pass_to_read() const {
			return _last_pass_to_read;
		}

		const FrameGraphPassBase* last_pass_to_write() const {
			return  _last_pass_to_write;
		}

		bool operator==(const FrameGraphResourceBase& other) const {
			return _id == other._id;
		}

	protected:
		friend class FrameGraphBuilder;
		friend class FrameGraphResources;

		static constexpr u32 invalid_id = u32(-1);

		u32 _id = invalid_id;
		const FrameGraphPassBase* _last_pass_to_read = nullptr;
		const FrameGraphPassBase* _last_pass_to_write = nullptr;

		PipelineStage _read_stage = PipelineStage::None;
		PipelineStage _write_stage = PipelineStage::None;
};

template<typename T>
class FrameGraphResource : public FrameGraphResourceBase {
	public:
		FrameGraphResource() = default;
};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHRECOURCE_H
