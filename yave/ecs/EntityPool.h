/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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
#ifndef YAVE_ECS_ENTITYPOOL_H
#define YAVE_ECS_ENTITYPOOL_H

#include "ecs.h"

#include <y/core/Vector.h>
#include <y/core/Range.h>

#include <y/reflect/reflect.h>

#include <y/utils/iter.h>

namespace yave {
namespace ecs {

class EntityPool : NonCopyable {
    struct Entity {
        EntityId id;
        EntityId parent;

        EntityId first_child;
        EntityId left_sibling;
        EntityId right_sibling;

        bool is_valid() const;
        void invalidate();
        void make_valid(u32 index);

        y_reflect(Entity, id, parent, first_child, left_sibling, right_sibling)
    };

    public:
        EntityPool() = default;
        EntityPool(EntityPool&&) = default;
        EntityPool& operator=(EntityPool&&) = default;

        usize size() const;
        bool exists(EntityId id) const;

        EntityId id_from_index(u32 index) const;

        EntityId create();
        void recycle(EntityId id);

        EntityId parent(EntityId id) const;
        void set_parent(EntityId id, EntityId parent_id);

        void audit();

        auto ids() const {
            return core::Range(
                FilterIterator(
                    TransformIterator(_entities.begin(), [](const Entity& e) { return e.id; }),
                    _entities.end(),
                    [](EntityId id) { return id.is_valid(); }
                ), EndIterator()
            );
        }

        y_reflect(EntityPool, _entities, _free)

    private:
        core::Vector<Entity> _entities;
        core::Vector<u32> _free;
};

}
}

#endif // YAVE_ECS_ENTITYPOOL_H

