/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#include "Query.h"

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {
namespace ecs {

core::Vector<EntityId> QueryUtils::matching(core::Span<const SparseIdSetBase*> sets, core::Span<EntityId> ids) {
    y_profile();

    auto match = core::vector_with_capacity<EntityId>(ids.size());
    for(EntityId id : ids) {
        bool contains = true;
        for(usize i = 0; contains && i != sets.size(); ++i) {
            y_debug_assert(sets[i]);
            contains &= sets[i]->contains(id);
        }

        if(contains) {
            match.push_back(id);
        }
    }

    return match;
}

}
}
