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
#ifndef YAVE_WORLD_ENTITY_H
#define YAVE_WORLD_ENTITY_H

#include "EntityId.h"
#include "ComponentRef.h"

#include <y/core/Vector.h>


namespace yave {

struct Entity : NonCopyable {
    EntityId id;
    EntityId parent;
};

class EntityPool {
    public:
        EntityId create_entity();
        void remove_entity(EntityId id);

        bool exists(EntityId id) const;

        usize entity_count() const;

    private:
        EntityId create_id();

        core::Vector<Entity> _entities;
        core::Vector<EntityId> _free;
};

}


#endif // YAVE_WORLD_ENTITY_H

