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
#include <y/core/Result.h>

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
        void invalidate_hierarchy();
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
        void remove(EntityId id);

        EntityId first_child(EntityId id) const;
        EntityId next_child(EntityId id) const;

        EntityId parent(EntityId id) const;
        void set_parent(EntityId id, EntityId parent_id);

        bool is_parent(EntityId id, EntityId parent) const;

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

        auto parents(EntityId id) const {
            return core::Range(
                FunctorIterator(
                    [this, i = id]() mutable -> core::Result<EntityId> {
                        i = parent(i);
                        if(!i.is_valid()) {
                            return core::Err();
                        }
                        return core::Ok(i);
                    }
                ), EndIterator()
            );
        }

        auto children(EntityId id) const {
            const EntityId start = first_child(id);
            return core::Range(
                FunctorIterator(
                    [this, i = start, start, init = false]() mutable -> core::Result<EntityId> {
                        if(!i.is_valid() || (i == start && init)) {
                            return core::Err();
                        }
                        init = true;
                        const EntityId prev = i;
                        i = next_child(prev);
                        return core::Ok(prev);
                    }
                ), EndIterator()
            );
        }

        y_reflect(EntityPool, _entities, _free)

    private:
        void reroot_all_children(EntityId id, EntityId new_parent);

        core::Vector<Entity> _entities;
        core::Vector<u32> _free;
};

}
}

#endif // YAVE_ECS_ENTITYPOOL_H

