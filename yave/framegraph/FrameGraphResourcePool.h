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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPHRESOURCEPOOL_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPHRESOURCEPOOL_H

#include <typeindex>

#include <yave/graphics/bindings/DescriptorSet.h>
#include <yave/graphics/framebuffer/Framebuffer.h>

#include "FrameGraphResource.h"
#include "FrameGraphPass.h"

namespace yave {


class FrameGraphResourcePool : public DeviceLinked, NonCopyable {

	struct ResourceContainerBase : NonCopyable {
		ResourceContainerBase(std::type_index t) : type(t) {
		}

		virtual ~ResourceContainerBase() = default;

		const std::type_index type;
	};

	template<typename T>
	struct ResourceContainer : ResourceContainerBase {
		template<typename... Args>
		ResourceContainer(Args&&... args) : ResourceContainerBase(typeid(T)), resource(y_fwd(args)...) {
		}

		T resource;
	};

	using ResourceConstructor = core::Function<std::unique_ptr<ResourceContainerBase>()>;

	public:
		FrameGraphResourcePool(DevicePtr dptr);

		usize resource_count() const;



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
		FrameGraphResource<T> add_generic_resource(Args&&... args) {
			FrameGraphResource<T> res;
			res._id = _resources_ctors.size();
			_resources_ctors.emplace_back([tpl = std::make_tuple(y_fwd(args)...)]() mutable {
					// extra layer of lambda to force the types of the arguments (overwise make_unique won't resolve properly)
					return std::apply([](auto&&... a) { return std::make_unique<ResourceContainer<T>>(y_fwd(a)...); }, std::move(tpl));
				});
			return res;
		}

		template<typename... Args>
		FrameGraphResource<DescriptorSet> add_descriptor_set(Args&&... args) {
			FrameGraphResource<DescriptorSet> res;
			res._id = _resources_ctors.size();
			_resources_ctors.emplace_back([tpl = std::make_tuple(y_fwd(args)...), this]() mutable {
					return std::apply([this](auto&&... a) {
						return std::make_unique<ResourceContainer<DescriptorSet>>(
							device(),
							core::ArrayView({forward_descriptor(y_fwd(a))...}
						));
					}, std::move(tpl));
				});
			return res;
		}

		template<typename D, typename... Args>
		FrameGraphResource<Framebuffer> add_framebuffer(D&& depth, Args&&... args) {
			FrameGraphResource<Framebuffer> res;
			res._id = _resources_ctors.size();
			_resources_ctors.emplace_back([tpl = std::make_tuple(y_fwd(depth), y_fwd(args)...), this]() mutable {
					return std::apply([this](auto&& d, auto&&... a) {
						return std::make_unique<ResourceContainer<Framebuffer>>(
							device(),
							forward_attachment<DepthAttachmentView>(d),
							core::ArrayView({forward_attachment<ColorAttachmentView>(y_fwd(a))...})
						);
					}, std::move(tpl));
				});
			return res;
		}

	private:
		template<typename T>
		Binding forward_descriptor(T&& res) {
			if constexpr(is_framegraph_resource<std::decay_t<T>>::value) {
				return Binding(get(y_fwd(res)));
			} else {
				return Binding(y_fwd(res));
			}
		}

		template<typename V, typename T>
		V forward_attachment(T&& res) {
			if constexpr(is_framegraph_resource<std::decay_t<T>>::value) {
				return V(get(y_fwd(res)));
			} else {
				return V(y_fwd(res));
			}
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

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHRESOURCEPOOL_H
