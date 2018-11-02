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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPH_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPH_H

#include "FrameGraphResource.h"
#include "FrameGraphPass.h"
#include "FrameGraphBuilder.h"

#include <set>

namespace yave {

class FrameGraph : NonCopyable {
	public:
		FrameGraph(DevicePtr dptr) : _data(std::make_shared<Data>(dptr)) {
		}

		template<typename T>
		T& add_callback_pass(std::string_view pass_name,
							 typename CallBackFrameGraphPass<T>::setup_func&& setup,
							 typename CallBackFrameGraphPass<T>::render_func&& render) {

			auto pass = std::make_unique<CallBackFrameGraphPass<T>>(pass_name, std::move(setup), std::move(render));
			CallBackFrameGraphPass<T>* ptr = pass.get();
			FrameGraphBuilder::build(ptr, _data->resources);
			 _data->passes << std::move(pass);
			return ptr->_data;
		}

		void render(CmdBufferRecorder& recorder, const FrameGraphResourceBase& output) && {
			for(const auto& pass : compile_sequential(output)) {
				auto region = recorder.region(pass->name());
				pass->render(recorder,  _data->resources);
			}
			recorder.keep_alive(_data);
		}

	private:
		core::Vector<const FrameGraphPassBase*> compile_sequential(const FrameGraphResourceBase& output) const {
			core::Vector<const FrameGraphPassBase*> passes;
			for(const auto& p : _data->passes) {
				if(p->uses_resource(output)) {
					fill_dependencies(passes, p.get());
				}
			}

			std::reverse(passes.begin(), passes.end());
			return passes;
		}

		void fill_dependencies(core::Vector<const FrameGraphPassBase*>& passes, const FrameGraphPassBase* pass) const {
			if(std::find(passes.begin(), passes.end(), pass) == passes.end()) {
				passes.push_back(pass);
				for(const auto& r : pass->used_resources()) {
					const FrameGraphPassBase* write = r.last_pass_to_write();
					if(write) {
						fill_dependencies(passes, write);
					}
				}
			}
		}

		struct Data {
			Data(DevicePtr dptr) : resources(dptr) {
			}

			FrameGraphResources resources;
			core::Vector<std::unique_ptr<FrameGraphPassBase>> passes;
		};

		std::shared_ptr<Data> _data;
};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPH_H
