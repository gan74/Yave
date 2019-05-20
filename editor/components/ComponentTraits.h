/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#ifndef EDITOR_PROPERTIES_PROPERTIES_H
#define EDITOR_PROPERTIES_PROPERTIES_H

#include <editor/editor.h>
#include <yave/ecs/EntityId.h>

#include <editor/components/EditorComponent.h>
#include <yave/components/LightComponent.h>
#include <yave/components/RenderableComponent.h>

namespace yave {
namespace ecs {
class EntityWorld;
}
}

namespace editor {

struct ComponentTraits {
	using ui_function_t = void (*)(ContextPtr, ecs::EntityId);

	std::type_index type = typeid(void);
	std::string_view name;
	ui_function_t widget = nullptr;
};

ComponentTraits component_traits(std::type_index type);
core::Vector<ComponentTraits> all_component_traits();

void component_widget(std::type_index type, ContextPtr ctx, ecs::EntityId id);

template<typename T>
ComponentTraits component_traits() {
	return component_traits(typeid(T));
}


}

#endif // EDITOR_PROPERTIES_PROPERTIES_H
