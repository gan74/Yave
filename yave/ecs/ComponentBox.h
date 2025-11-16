/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#ifndef YAVE_ECS_COMPONENTBOX_H
#define YAVE_ECS_COMPONENTBOX_H

#include "ecs.h"
#include "ComponentRuntimeInfo.h"

#include <yave/assets/AssetPtr.h>

#include <y/core/AssocVector.h>
#include <y/serde3/archives.h>

namespace yave {
namespace ecs {

using EntityIdMap = core::AssocVector<EntityId, EntityId>;

class ComponentBoxBase : NonMovable {
    public:
        virtual ~ComponentBoxBase() = default;

        virtual ComponentRuntimeInfo runtime_info() const = 0;
        virtual void add_to(EntityWorld& world, EntityId id, const EntityIdMap& id_map) const = 0;
        virtual void add_or_replace(EntityWorld& world, EntityId id) const = 0;

        y_serde3_poly_abstract_base(ComponentBoxBase)
};

template<typename T>
class ComponentBox final : public ComponentBoxBase {
    public:
        ComponentBox() = default;
        ComponentBox(T t);

        ComponentRuntimeInfo runtime_info() const override;
        void add_to(EntityWorld& world, EntityId id, const EntityIdMap& id_map) const override;
        void add_or_replace(EntityWorld& world, EntityId id) const override;

        const T& component() const {
            return _component;
        }

        y_reflect(ComponentBox, _component)
        y_serde3_poly(ComponentBox)

    private:
        T _component;
};

}
}

#endif // YAVE_ECS_COMPONENTBOX_H

