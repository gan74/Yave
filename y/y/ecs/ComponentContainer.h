/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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
#ifndef Y_ECS_COMPONENTCONTAINER_H
#define Y_ECS_COMPONENTCONTAINER_H

#include "ecs.h"

#include "ComponentRuntimeInfo.h"

#include <y/serde3/archives.h>

#include <memory>

namespace y {
namespace ecs {

class ComponentContainerBase {
	public:
		virtual ~ComponentContainerBase() {
		}

		virtual ComponentRuntimeInfo create_runtime_info() const;

		virtual void* data() = 0;

		y_serde3_poly_base(ComponentContainerBase)
};

template<typename T>
class ComponentContainer : public ComponentContainerBase {
	public:
		static_assert(std::is_copy_constructible_v<T>);

		ComponentContainer() = default;

		ComponentContainer(const void* orig) : _component(*static_cast<const T*>(orig)) {
		}

		ComponentRuntimeInfo create_runtime_info() const override {
			return ComponentRuntimeInfo::from_type<T>();
		}

		void* data() override {
			return &_component;
		}

		y_serde3_poly(ComponentContainer);
		y_serde3(_component);

	private:
		T _component = {};
};

namespace detail {
template<typename T>
std::unique_ptr<ComponentContainerBase> create_component_container(const void* orig) {
	return std::make_unique<ComponentContainer<T>>(orig);
}
}

}
}

#endif // Y_ECS_COMPONENTCONTAINER_H
