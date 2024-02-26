/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef YAVE_ECS2_COMPONENTCONTAINER_H
#define YAVE_ECS2_COMPONENTCONTAINER_H

#include "ComponentRuntimeInfo.h"
#include "ComponentMatrix.h"

#include <y/serde3/poly.h>

namespace yave {
namespace ecs2 {

class ComponentContainerBase : NonMovable {
    public:
        virtual ~ComponentContainerBase();

        ComponentTypeIndex type_id() const;

        y_serde3_poly_abstract_base(ComponentContainerBase)

    protected:
        friend class EntityWorld;

        ComponentContainerBase(ComponentTypeIndex type_id) : _type_id(type_id) {
        }

        const ComponentTypeIndex _type_id;
        ComponentMatrix* _matrix = nullptr;
};


template<typename T>
class ComponentContainer final : public ComponentContainerBase {
    using Slot = core::SlotVector<T>::Slot;

    public:
        using component_type = T;

        ComponentContainer() : ComponentContainerBase(type_index<T>()) {
        }


        T* get_or_add(EntityId id) {
            if(const Slot slot = _matrix->component_slot_index<T>(id); slot != Slot::invalid_slot) {
                return &_components[slot];
            }
            return add(id);
        }

        template<typename... Args>
        T* add_or_replace(EntityId id, Args&... args) {
            if(const Slot slot = _matrix->component_slot_index<T>(id); slot != Slot::invalid_slot) {
                return &(_components[slot] = T(y_fwd(args)...));
            }
            return add(id, y_fwd(args)...);
        }


        y_serde3_poly(ComponentContainer)
        y_reflect(ComponentContainer, _components)

    private:
        friend class EntityWorld;

        template<typename... Args>
        T* add(EntityId id, Args&... args) {
            const Slot slot = _components.insert(y_fwd(args)...);
            _matrix->add_component<T>(id, slot);
            return &_components[slot];
        }

        core::SlotVector<T> _components;
};

}
}

#endif // YAVE_ECS2_COMPONENTCONTAINER_H

