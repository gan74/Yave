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

#include "World.h"

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

QueryResult World::compute_query(core::Span<ComponentLut*> luts) {
    QueryResult result;
    if(luts.is_empty()) {
        return result;
    }

    usize smallest_index = 0;
    usize smallest_size = usize(-1);
    {
        for(usize i = 0; i != luts.size(); ++i) {
            if(!luts[i]) {
                return result;
            }

            const usize size = luts[i]->size();
            if(size < smallest_size) {
                smallest_size = size;
                smallest_index = i;
            }
        }
    }

    core::Span<ComponentLut::Entry> base;
    core::ScratchPad<core::Span<ComponentLut::Entry>> iterators(luts.size() - 1);
    for(usize i = 0; i != luts.size(); ++i) {
        y_debug_assert(std::is_sorted(luts[i]->lut().begin(), luts[i]->lut().end()));
        if(i == smallest_index) {
            base = luts[i]->lut();
        } else {
            iterators[i < smallest_index ? i : (i - 1)] = luts[i]->lut();
        }
    }


    for(const auto& base_it : base) {
        const EntityId id = base_it.id;

        bool matching = true;
        for(auto& it : iterators) {
            while(!it.is_empty() && it[0].id < id) {
                it = it.take(1);
            }
            if(it.is_empty()) {
                matching = false;
                break;
            }
            matching &= (id == it[0].id);
        }

        if(matching) {
            result.ids << id;

            for(usize i = 0; i != smallest_index; ++i) {
                result.refs << iterators[i][0].ref;
            }

            result.refs << base_it.ref;

            for(usize i = smallest_index; i != iterators.size(); ++i) {
                result.refs << iterators[i][0].ref;
            }
        }
    }

    return result;
}


}

