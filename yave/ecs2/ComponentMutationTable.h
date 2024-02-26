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
#ifndef YAVE_ECS2_COMPONENTMUTATIONTABLE_H
#define YAVE_ECS2_COMPONENTMUTATIONTABLE_H

#include "ecs.h"

#include <y/core/Vector.h>

namespace yave {
namespace ecs2 {

class ComponentMutationTable : NonMovable {
    public:
        inline void set_min_size(EntityId id) {
            const u32 index = id.index();
            const u32 pack_index = index / 64;
            _bits.set_min_size(pack_index + 1);
        }

        inline bool add(EntityId id) {
            const u32 index = id.index();
            const u32 pack_index = index / 64;
            const u32 bit_index = index % 64;
            const u64 mask = u64(1) << bit_index;

            y_debug_assert(pack_index < _bits.size());

            if((_bits[pack_index] & mask) == 0) {
                _bits[pack_index] |= mask;
                _ids << id;
                return true;
            }
            return false;
        }

        inline bool contains(EntityId id) {
            const u32 index = id.index();
            const u32 pack_index = index / 64;
            const u32 bit_index = index % 64;
            const u64 mask = u64(1) << bit_index;
            return pack_index < _bits.size() && (_bits[pack_index] & mask) != 0;
        }

        inline void make_empty() {
            _ids.make_empty();
            std::fill(_bits.begin(), _bits.end(), 0);
        }

        inline usize size() const {
            return _ids.size();
        }

        inline core::Span<EntityId> ids() const {
            return _ids;
        }

    private:
        core::Vector<u64> _bits;
        core::Vector<EntityId> _ids;
};

}
}

#endif // YAVE_ECS2_COMPONENTMUTATIONTABLE_H

