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
#ifndef YAVE_RENDERGRAPH_RENDERGRAPHRECOURCE_H
#define YAVE_RENDERGRAPH_RENDERGRAPHRECOURCE_H

#include <yave/yave.h>

#include <typeindex>

namespace yave {

class RenderGraphBuilder;
class RenderGraphResources;

class RenderGraphResourceBase {
	public:
		enum class UsageFlags {

		};

		RenderGraphResourceBase() = default;

		u32 id() const {
			return _id;
		}

		bool is_valid() const {
			return _id != invalid_id;
		}

		bool is_initialized() const {
			return _last_op != Undefined;
		}

		u32 last_pass_index() const {
			return _pass_index;
		}

		bool is_read() const {
			return _last_op == Read;
		}

		bool operator==(const RenderGraphResourceBase& other) const {
			return _id == other._id && _version == other._version;
		}

	protected:
		friend class RenderGraphBuilder;
		friend class RenderGraphResources;

		static constexpr u32 invalid_id = u32(-1);

		u32 _id = invalid_id;
		u32 _version = 0;
		u32 _pass_index = u32(-1);

		enum {
			Undefined,
			Read,
			Write
		} _last_op = Undefined;

		PipelineStage _last_op_stage = PipelineStage::None;
};

template<typename T>
class RenderGraphResource : public RenderGraphResourceBase {
	public:
		RenderGraphResource() = default;
};

}

#endif // YAVE_RENDERGRAPH_RENDERGRAPHRECOURCE_H
