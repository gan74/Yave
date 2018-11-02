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

#include "FrameGraphResource.h"
#include "FrameGraphPass.h"

namespace yave {

class FrameGraphResources : public DeviceLinked, NonCopyable {

	struct ResourceContainerBase : NonCopyable {
		ResourceContainerBase(std::type_index t) : type(t) {
		}

		virtual ~ResourceContainerBase() = default;

		const std::type_index type;
	};

	template<typename T>
	struct ResourceContainer : ResourceContainerBase {
		template<typename... Args>
		ResourceContainer(Args&&... args) : ResourceContainerBase(typeid(T)), resource(std::forward<Args>(args)...) {
		}

		T resource;
	};

	using ResourceConstructor = core::Function<std::unique_ptr<ResourceContainerBase>()>;

	public:
		FrameGraphResources(DevicePtr dptr) : DeviceLinked(dptr) {
		}

		template<typename T>
		T& get(const FrameGraphResource<T>& res) const {
			if(!res.is_valid()) {
				y_fatal("Invalid resource.");
			}
			while(_resources.size() <= res.id()) {
				_resources.emplace_back();
			}
			init(res);
			ResourceContainerBase* resource = _resources[res.id()].get();
			if(resource->type != typeid(T)) {
				y_fatal("Invalid resource type.");
			}
			return dynamic_cast<ResourceContainer<T>*>(resource)->resource;
		}

		template<typename T, typename... Args>
		FrameGraphResource<T> add_resource(Args&&... args) {
			FrameGraphResource<T> res;
			res._id = _resources_ctors.size();

			_resources_ctors.emplace_back([tpl = std::make_tuple(std::forward<Args>(args)...)]() mutable {
					// extra layer of lambda to force the types of the arguments (overwise make_unique won't resolve properly)
					return std::apply([](Args&&... a) { return std::make_unique<ResourceContainer<T>>(std::forward<Args>(a)...); }, std::move(tpl));
				});
			return res;
		}

		usize resource_count() const {
			return _resources_ctors.size();
		}

	private:
		template<typename T>
		void init(const FrameGraphResource<T>& res) const {
			auto& r = _resources[res.id()];
			if(!r) {
				r = _resources_ctors[res.id()]();
			}
		}

		mutable core::Vector<std::unique_ptr<ResourceContainerBase>> _resources;
		core::Vector<ResourceConstructor> _resources_ctors;
};

class FrameGraphBuilder : NonCopyable {
	public:
		static void build(FrameGraphPassBase* pass, FrameGraphResources& res) {
			FrameGraphBuilder builder(pass, res);
			pass->setup(builder);
		}

		DevicePtr device() const {
			return _resources.device();
		}

		const FrameGraphResources& resources() const {
			return _resources;
		}


		template<typename T, typename... Args>
		FrameGraphResource<T> create(Args&&... args) {
			return _resources.add_resource<T>(std::forward<Args>(args)...);
		}

		template<typename T, typename... Args>
		void create(FrameGraphResource<T>& res, Args&&... args) {
			res = create<T>(std::forward<Args>(args)...);
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
		FrameGraphBuilder(FrameGraphPassBase* pass, FrameGraphResources& res) : _pass(pass), _resources(res) {
		}

		template<typename T>
		void check_res(const FrameGraphResource<T>& res) {
			if(!res.is_valid()) {
				y_fatal(fmt("Pass \"%\" use an invalid resource.", _pass->name()).data());
			}
		}

		FrameGraphPassBase* _pass = nullptr;
		FrameGraphResources& _resources;
};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHBUILDER_H
