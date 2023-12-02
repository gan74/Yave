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

#include "ComponentContainer.h"

namespace yave {
namespace ecs {

ComponentBoxBase::~ComponentBoxBase() {
}

ComponentContainerBase::~ComponentContainerBase() {
}

bool ComponentContainerBase::contains(EntityId id) const {
    return id_set().contains(id);
}

ComponentTypeIndex ComponentContainerBase::type_id() const {
    return _type_id;
}

const SparseIdSetBase& ComponentContainerBase::id_set() const {
    Y_TODO(clean this mess)
    return *reinterpret_cast<const SparseIdSetBase*>(this + 1);
}

const SparseIdSet& ComponentContainerBase::recently_mutated() const {
    return _mutated;
}

}
}

