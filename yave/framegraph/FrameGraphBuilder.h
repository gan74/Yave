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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPHBUILDER_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPHBUILDER_H

#include <typeindex>

#include "FrameGraphPassBuilder.h"
#include "FrameGraphPass.h"

namespace yave {

class FrameGraph;
//class FrameGraphBuilder;

class FrameGraphData : NonCopyable {
	public:
		FrameGraphData(FrameGraphResourcePool* res) : resources(res) {
		}

	private:
		friend class FrameGraph;
		//friend class FrameGraphBuilder;

		FrameGraphResourcePool* resources = nullptr;
		core::Vector<std::unique_ptr<FrameGraphPassBase>> passes;
};

/*class FrameGraphBuilder : NonCopyable {
	public:
		FrameGraphBuilder(FrameGraphResourcePool* resources) : _data(std::make_shared<FrameGraphData>(resources)) {
		}

		template<typename T>
		T& add_callback_pass(std::string_view pass_name,
							 typename CallBackFrameGraphPass<T>::setup_func&& setup,
							 typename CallBackFrameGraphPass<T>::render_func&& render) {

			auto pass = std::make_unique<CallBackFrameGraphPass<T>>(pass_name, std::move(setup), std::move(render));
			CallBackFrameGraphPass<T>* ptr = pass.get();
			FrameGraphPassBuilder::build(ptr, _data->resources);
			 _data->passes << std::move(pass);
			return ptr->_data;
		}

	private:
		friend class FrameGraph;

		std::shared_ptr<FrameGraphData> _data;
};*/

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHBUILDER_H
