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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPHPASS_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPHPASS_H

#include <y/core/Functor.h>

#include <yave/graphics/images/Image.h>
#include <yave/graphics/buffers/buffers.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include "FrameGraphResource.h"

namespace yave {

class FrameGraphPass : NonCopyable {
	public:
		struct ResourceUsageInfo {
			PipelineStage stage = PipelineStage::None;
		};

		using render_func = core::Function<void(CmdBufferRecorder&, const FrameGraphPass*)>;

		FrameGraphPass(std::string_view name, FrameGraph* parent);

		const core::String& name() const;
		const FrameGraphResourcePool* resources() const;
		const Framebuffer& framebuffer() const;

		void render(CmdBufferRecorder& recorder) const;


		const auto& all_images() const {
			return _images;
		}

		const auto& all_buffers() const {
			return _buffers;
		}


	private:
		friend class FrameGraph;
		friend class FrameGraphPassBuilder;

		render_func _render = [](CmdBufferRecorder&, const FrameGraphPass*) {};
		core::String _name;

		FrameGraph* _parent = nullptr;

		std::unordered_map<FrameGraphImage, ResourceUsageInfo> _images;
		std::unordered_map<FrameGraphBuffer, ResourceUsageInfo> _buffers;

		FrameGraphImage _depth;
		core::Vector<FrameGraphImage> _colors;

		Framebuffer _framebuffer;
};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHPASS_H
