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
#ifndef YAVE_WORLD_ENTITYID_H
#define YAVE_WORLD_ENTITYID_H

#include <yave/yave.h>

namespace yave {

class EntityId {
    public:
        u32 index() const {
            return _index;
        }

        bool is_valid() const {
            return _generation;
        }

        u64 as_u64() const {
            return (u64(_index) << 32) | u64(_generation);
        }

        bool operator==(const EntityId& other) const {
            return _index == other._index && _generation == other._generation;
        }

        bool operator!=(const EntityId& other) const {
            return !operator==(other);
        }

        bool operator<(const EntityId& other) const {
            return as_u64() < other.as_u64();
        }

    private:
        friend class EntityContainer;

        u32 _index = 0;
        u32 _generation = 0;
};

}


#endif // YAVE_WORLD_ENTITYID_H

