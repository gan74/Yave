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
#ifndef YAVE_ECS2_ENTITYGROUP_H
#define YAVE_ECS2_ENTITYGROUP_H

#include "ecs.h"

#include <y/core/Vector.h>

namespace yave {
namespace ecs2 {

class EntityGroupBase : NonMovable {
    public:
        virtual ~EntityGroupBase() = default;

        void add_entity(EntityId id) {
            _ids << id;
        }

        bool remove_entity(EntityId id) {
            if(const auto it = std::find(_ids.begin(), _ids.end(), id); it != _ids.end()) {
                _ids.erase_unordered(it);
                return true;
            }
            return false;
        }

        core::Span<ComponentTypeIndex> types() const {
            return _types;
        }

        core::Span<EntityId> ids() const {
            return _ids;
        }

    protected:
        EntityGroupBase(core::Span<ComponentTypeIndex> types) : _types(types) {
        }

        core::Span<ComponentTypeIndex> _types;
        core::Vector<EntityId> _ids;
};

template<typename... Ts>
class EntityGroup final : public EntityGroupBase {
    static constexpr usize type_count = sizeof...(Ts);
    public:
        EntityGroup() : EntityGroupBase(_types_storage) {
        }

    private:
        std::array<ComponentTypeIndex, type_count> _types_storage = { type_index<Ts>()... };
};


}
}

#endif // YAVE_ECS2_ENTITYGROUP_H

