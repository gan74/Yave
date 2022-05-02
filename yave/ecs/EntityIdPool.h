/*******************************
Copyright (c) 2016-2022 Gr�goire Angerand

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
#ifndef YAVE_ECS_ENTITYIDPOOL_H
#define YAVE_ECS_ENTITYIDPOOL_H

#include "ecs.h"

#include <y/core/Vector.h>
#include <y/core/Range.h>

#include <y/reflect/reflect.h>

#include <y/utils/iter.h>

namespace yave {
namespace ecs {

class EntityIdPool {
    public:
        EntityIdPool() = default;
        EntityIdPool(EntityIdPool&&) = default;
        EntityIdPool& operator=(EntityIdPool&&) = default;

        usize size() const;
        bool contains(EntityId id) const;

        EntityId id_from_index(u32 index) const;

        EntityId create();
        void recycle(EntityId id);


        auto ids() const {
            return core::Range(
                FilterIterator(_ids.begin(), _ids.end(), [](EntityId id) { return id.is_valid(); }),
                EndIterator()
            );
        }

        y_reflect(_ids, _free)

    private:
        core::Vector<EntityId> _ids;
        core::Vector<u32> _free;
};

}
}

#endif // YAVE_ECS_ENTITYIDPOOL_H

