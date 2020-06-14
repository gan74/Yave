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
#ifndef YAVE_ECS_COMPONENTRUNTIMEINFO_H
#define YAVE_ECS_COMPONENTRUNTIMEINFO_H

#include "ecs.h"

#include <y/serde3/serde.h>

#include <y/utils/name.h>

#include <memory>

namespace yave {
namespace ecs {

template<typename T>
std::unique_ptr<ComponentContainerBase> create_container();

struct ComponentRuntimeInfo {
	ComponentTypeIndex type_id;
	std::string_view type_name;
	std::unique_ptr<ComponentContainerBase> (*create_type_container)() = nullptr;

	template<typename T>
	static ComponentRuntimeInfo create() {
		return ComponentRuntimeInfo {
			type_index<T>(),
			ct_type_name<T>(),
			create_container<T>
		};
	}

	y_no_serde3()
};

}
}

#endif // YAVE_ECS_COMPONENTRUNTIMEINFO_H
