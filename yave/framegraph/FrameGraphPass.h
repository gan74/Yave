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

class FrameGraphResourcePool;

class FrameGraphPassBase : NonCopyable {
	public:

		virtual ~FrameGraphPassBase() = default;

		virtual void setup(FrameGraphPassBuilder& builder) = 0;
		virtual void render(CmdBufferRecorder& recorder, const FrameGraphResourcePool* resources) const = 0;

		bool uses_resource(const FrameGraphResourceBase& res) const {
			return std::find(_resources.begin(), _resources.end(), res) != _resources.end();
		}

		const core::Vector<FrameGraphResourceBase>& used_resources() const {
			return _resources;
		}

		const core::String& name() const {
			return _name;
		}

	protected:
		FrameGraphPassBase(std::string_view name) : _name(name) {
		}

	private:
		friend class FrameGraph;
		friend class FrameGraphPassBuilder;

		core::String _name;

		core::Vector<FrameGraphResourceBase> _resources;
};

template<typename T>
class CallBackFrameGraphPass final : public FrameGraphPassBase {
	public:
		using setup_func = core::Function<void(FrameGraphPassBuilder&, T&)>;
		using render_func = core::Function<void(CmdBufferRecorder& recorder, const T&, const FrameGraphResourcePool*)>;

		CallBackFrameGraphPass(std::string_view name, setup_func&& setup, render_func&& render) : FrameGraphPassBase(name), _render(std::move(render)), _setup(std::move(setup)) {
		}

		void setup(FrameGraphPassBuilder& builder) override {
			_setup(builder, _data);
		}

		void render(CmdBufferRecorder& recorder, const FrameGraphResourcePool* resources) const override {
			_render(recorder, _data, resources);
		}

	private:
		friend class FrameGraph;
		//friend class FrameGraphBuilder;

		render_func _render;
		setup_func _setup;

		T _data;
};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHPASS_H
