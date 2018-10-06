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
#ifndef YAVE_RENDERGRAPH_RENDERGRAPHBUILDER_H
#define YAVE_RENDERGRAPH_RENDERGRAPHBUILDER_H

#include <typeindex>

#include "RenderGraphResource.h"
#include "RenderGraphPass.h"

namespace yave {

class RenderGraphResources : public DeviceLinked, NonCopyable {

	struct ResourceContainerBase : NonCopyable {
		virtual ~ResourceContainerBase() = default;

		ResourceContainerBase(std::type_index t) : type(t) {
		}

		const std::type_index type;
	};

	template<typename T>
	struct ResourceContainer : ResourceContainerBase {
		template<typename... Args>
		ResourceContainer(Args&&... args) : ResourceContainerBase(typeid(T)), resource(std::forward<Args>(args)...) {
		}

		T resource;

	};

	using ResourceConstructor = core::Function<std::unique_ptr<ResourceContainerBase>(DevicePtr)>;

	public:
		RenderGraphResources(DevicePtr dptr) : DeviceLinked(dptr) {
		}

		template<typename T>
		T& get(const RenderGraphResource<T>& res) {
			while(_resources.size() <= res.id()) {
				_resources.emplace_back();
			}
			init(res);
			ResourceContainerBase* resource = _resources[res.id()].get();
			if(resource->type != typeid(T)) {
				fatal("Invalid resource type.");
			}
			return dynamic_cast<ResourceContainer<T>*>(resource)->resource;
		}

		template<typename T, typename... Args>
		RenderGraphResource<T> create(Args&&... args) {
			RenderGraphResource<T> res;
			res._id = _resources_ctors.size();
			_resources_ctors.emplace_back([tpl = std::make_tuple(std::forward<Args>(args)...)](DevicePtr) mutable {
					return std::apply(std::make_unique<ResourceContainer<T>>, std::move(tpl));
				});
			return res;
		}

		usize resource_count() const {
			return _resources_ctors.size();
		}

	private:
		template<typename T>
		void init(const RenderGraphResource<T>& res) {
			auto& r = _resources[res.id()];
			if(!r) {
				r = *_resources_ctors[res.id()](device());
			}
		}

		core::Vector<std::unique_ptr<ResourceContainerBase>> _resources;
		core::Vector<ResourceConstructor> _resources_ctors;
};

class RenderGraphBuilder : NonCopyable {
	public:
		static void build(RenderGraphPassBase* pass, RenderGraphResources& res) {
			RenderGraphBuilder builder(pass, res);
			pass->setup(builder);
		}

		template<typename T, typename... Args>
		RenderGraphResource<T> create(Args&&... args) {
			RenderGraphResource<T> res = _resources.create<T>(std::forward<Args>(args)...);
			res._pass_index = _pass->index();
			_pass->_resources << res;
			return res;
		}

		template<typename T, typename... Args>
		void create(RenderGraphResource<T>& res, Args&&... args) {
			res = create<T>(std::forward<Args>(args)...);
		}

		template<typename T>
		void read(RenderGraphResource<T>& res, PipelineStage stage = PipelineStage::EndOfPipe) {
			check_res(res);
			if(!res.is_initialized()) {
				fatal("Uninitialized resource read.");
			}
			res._pass_index = _pass->index();
			res._last_op = RenderGraphResource<T>::Read;
			res._last_op_stage = stage;
			_pass->_resources << res;
		}

		template<typename T>
		void render_to(RenderGraphResource<T>& res, PipelineStage stage = PipelineStage::ColorAttachmentOutBit) {
			check_res(res);
			++res._version;
			res._pass_index = _pass->index();
			res._last_op = RenderGraphResource<T>::Write;
			res._last_op_stage = stage;
			_pass->_resources << res;
		}

		template<typename T>
		void write(RenderGraphResource<T>& res, PipelineStage stage = PipelineStage::BeginOfPipe) {
			return render_to(res, stage);
		}

	private:
		RenderGraphBuilder(RenderGraphPassBase* pass, RenderGraphResources& res) : _pass(pass), _resources(res) {
		}

		template<typename T>
		void check_res(const RenderGraphResource<T>& res) {
			if(!res.is_valid()) {
				fatal("Render graph resource has not been initialized.");
			}
		}

		RenderGraphPassBase* _pass = nullptr;
		RenderGraphResources& _resources;
};

}

#endif // YAVE_RENDERGRAPH_RENDERGRAPHBUILDER_H
