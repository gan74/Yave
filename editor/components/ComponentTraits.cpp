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

#include "ComponentTraits.h"

#include "widgets.h"

#include <imgui/yave_imgui.h>

namespace editor {

struct RegisteredComponentType {
	ComponentTraits traits;
	RegisteredComponentType* next = nullptr;
};

static RegisteredComponentType* registered_types_head = nullptr;

void register_component_type(RegisteredComponentType* type, ComponentTraits traits) {
	for(auto* i = registered_types_head; i; i = i->next) {
		y_debug_assert(i->traits.type != traits.type);
	}
	y_debug_assert(!type->next);

	type->traits = traits;
	type->next = registered_types_head;
	registered_types_head = type;
}


ComponentTraits component_traits(std::type_index type) {
	for(auto* i = registered_types_head; i; i = i->next) {
		if(i->traits.type == type) {
			return i->traits;
		}
	}
	return ComponentTraits();
}

core::Vector<ComponentTraits> all_component_traits() {
	y_profile();
	core::Vector<ComponentTraits> traits;
	for(auto* i = registered_types_head; i; i = i->next) {
		traits << i->traits;
	}
	return traits;
}


void component_widget(std::type_index type, ContextPtr ctx, ecs::EntityId id) {
	ComponentTraits traits = component_traits(type);
	if(traits.widget) {
		traits.widget(ctx, id);
	}
}


#define EDITOR_COMPONENT_REGISTERER y_create_name_with_prefix(component)
#define REGISTER_COMPONENT_TYPE(Type)																			\
	namespace {																									\
		class EDITOR_COMPONENT_REGISTERER {																		\
			EDITOR_COMPONENT_REGISTERER() {																		\
				register_component_type(&type, ComponentTraits{typeid(Type), #Type, &widget<Type>});			\
			}																									\
			static EDITOR_COMPONENT_REGISTERER registerer;														\
			RegisteredComponentType type;																		\
		};																										\
		EDITOR_COMPONENT_REGISTERER EDITOR_COMPONENT_REGISTERER::registerer = EDITOR_COMPONENT_REGISTERER();	\
	}


REGISTER_COMPONENT_TYPE(EditorComponent)
REGISTER_COMPONENT_TYPE(LightComponent)
REGISTER_COMPONENT_TYPE(RenderableComponent)


#undef EDITOR_COMPONENT_REGISTERER
#undef REGISTER_COMPONENT_TYPE

}
