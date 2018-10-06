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
#ifndef YAVE_RENDERGRAPH_RENDERGRAPHPASS_H
#define YAVE_RENDERGRAPH_RENDERGRAPHPASS_H

#include <y/core/Functor.h>

#include <yave/graphics/images/Image.h>
#include <yave/graphics/buffers/buffers.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include "RenderGraphResource.h"

namespace yave {

class RenderGraphResources;

class RenderGraphPassBase : NonCopyable {
	public:

		virtual ~RenderGraphPassBase() = default;

		virtual void setup(RenderGraphBuilder& builder) = 0;
		virtual void render(CmdBufferRecorderBase& recorder, const RenderGraphResources& resources) const = 0;

		u32 index() const {
			return _index;
		}

		bool writes_resource(const RenderGraphResourceBase& res) const {
			for(const auto& r : _resources) {
				if(r == res) {
					return true;
				}
			}
			return false;
		}

		const core::Vector<RenderGraphResourceBase>& used_resources() const {
			return _resources;
		}

	protected:
		RenderGraphPassBase(u32 index) : _index(index) {
		}

	private:
		friend class RenderGraph;
		friend class RenderGraphBuilder;

		u32 _index = u32(-1);

		core::Vector<RenderGraphResourceBase> _resources;
};

template<typename T>
class RenderGraphPass final : public RenderGraphPassBase {
	public:
		using setup_func = core::Function<void(RenderGraphBuilder&, T&)>;
		using render_func = core::Function<void(CmdBufferRecorderBase& recorder, const T& , const RenderGraphResources&)>;

		RenderGraphPass(u32 index, setup_func&& setup, render_func&& render) : RenderGraphPassBase(index), _render(std::move(render)), _setup(std::move(setup)) {
		}

		void setup(RenderGraphBuilder& builder) override {
			_setup(builder, _data);
		}

		void render(CmdBufferRecorderBase& recorder, const RenderGraphResources& resources) const override {
			_render(recorder, _data, resources);
		}

	private:
		friend class RenderGraph;

		render_func _render;
		setup_func _setup;

		T _data;
};

}

#endif // YAVE_RENDERGRAPH_RENDERGRAPHPASS_H
