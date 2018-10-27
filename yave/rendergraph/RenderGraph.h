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
#ifndef YAVE_RENDERGRAPH_RENDERGRAPH_H
#define YAVE_RENDERGRAPH_RENDERGRAPH_H

#include "RenderGraphResource.h"
#include "RenderGraphPass.h"
#include "RenderGraphBuilder.h"

#include <set>

namespace yave {

class RenderGraph : NonCopyable {
	public:
		RenderGraph(DevicePtr dptr) : _resources(dptr) {
		}

		template<typename T>
		T& add_callback_pass(typename RenderGraphPass<T>::setup_func&& setup,
							 typename RenderGraphPass<T>::render_func&& render) {

			u32 pass_index = u32(_passes.size());
			auto pass = std::make_unique<RenderGraphPass<T>>(pass_index, std::move(setup), std::move(render));
			RenderGraphPass<T>* ptr = pass.get();
			RenderGraphBuilder::build(ptr, _resources);
			_passes << std::move(pass);
			return ptr->_data;
		}

		auto compile_sequential(const RenderGraphResourceBase& output) const {
			std::set<const RenderGraphPassBase*> passes;
			for(const auto& p : _passes) {
				if(p->writes_resource(output)) {
					fill_dependencies(passes, p.get());
				}
			}
			return passes;

			/*auto v = core::vector_with_capacity<const RenderGraphPassBase*>(passes.size());
			std::copy(passes.begin(), passes.end(), std::back_inserter(v));
			return v;*/
		}

		void render(CmdBufferRecorder& recorder, const RenderGraphResourceBase& output) {
			for(const auto& pass : compile_sequential(output)) {
				pass->render(recorder, _resources);
			}
		}

	private:
		template<template<typename...> typename Set>
		void fill_dependencies(Set<const RenderGraphPassBase*>& passes, const RenderGraphPassBase* pass) const {
			if(passes.find(pass) == passes.end()) {
				passes.insert(pass);
				for(const auto& r : pass->used_resources()) {
					if(r.is_read()) {
						fill_dependencies(passes, _passes[r.last_pass_index()].get());
					}
				}
			}
		}


		RenderGraphResources _resources;
		core::Vector<std::unique_ptr<RenderGraphPassBase>> _passes;
};

}

#endif // YAVE_RENDERGRAPH_RENDERGRAPH_H
