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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPHPASSBUILDER_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPHPASSBUILDER_H

#include "FrameGraphResourcePool.h"

namespace yave {

class FrameGraphPassBuilder : NonCopyable {
	public:
		static void build(FrameGraphPassBase* pass, FrameGraphResourcePool* res) {
			FrameGraphPassBuilder builder(pass, res);
			pass->setup(builder);
		}

		DevicePtr device() const {
			return _resources->device();
		}

		const FrameGraphResourcePool* resources() const {
			return _resources;
		}



		template<typename T, typename... Args>
		FrameGraphResource<T> declare(Args&&... args) {
			return _resources->add_generic_resource<T>(y_fwd(args)...);
		}

		template<typename T, typename... Args>
		void declare(FrameGraphResource<T>& res, Args&&... args) {
			res = declare<T>(y_fwd(args)...);
		}


		template<typename... Args>
		FrameGraphResource<DescriptorSet> declare_descriptor_set(Args&&... args) {
			return _resources->add_descriptor_set(y_fwd(args)...);
		}

		template<typename... Args>
		void declare_descriptor_set(FrameGraphResource<DescriptorSet>& res, Args&&... args) {
			res = declare_descriptor_set(y_fwd(args)...);
		}

		template<typename... Args>
		FrameGraphResource<Framebuffer> declare_framebuffer(Args&&... args) {
			return _resources->add_framebuffer(y_fwd(args)...);
		}

		template<typename... Args>
		void declare_framebuffer(FrameGraphResource<Framebuffer>& res, Args&&... args) {
			res = declare_framebuffer(y_fwd(args)...);
		}



		template<typename T>
		void read(FrameGraphResource<T>& res, PipelineStage stage = PipelineStage::EndOfPipe) {
			check_res(res);
			if(!res.is_initialized()) {
				y_fatal(fmt("Pass \"%\" reads an uninitialized resource.", _pass->name()).data());
			}
			_pass->_resources << res;
			res._last_pass_to_read = _pass;
			res._read_stage = stage;
		}

		template<typename T>
		void render_to(FrameGraphResource<T>& res, PipelineStage stage = PipelineStage::ColorAttachmentOutBit) {
			_pass->_resources << res;
			res._last_pass_to_write = _pass;
			res._write_stage = stage;
		}

		template<typename T>
		void write(FrameGraphResource<T>& res, PipelineStage stage = PipelineStage::BeginOfPipe) {
			return render_to(res, stage);
		}

	private:
		FrameGraphPassBuilder(FrameGraphPassBase* pass, FrameGraphResourcePool* res) : _pass(pass), _resources(res) {
		}

		template<typename T>
		void check_res(const FrameGraphResource<T>& res) {
			if(!res.is_valid()) {
				y_fatal(fmt("Pass \"%\" use an invalid resource.", _pass->name()).data());
			}
		}

		FrameGraphPassBase* _pass = nullptr;
		FrameGraphResourcePool* _resources = nullptr;
};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHBUILDER_H


