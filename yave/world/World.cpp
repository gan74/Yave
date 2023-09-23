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

std::strong_ordering ComponentLut::Entry::operator<=>(const Entry& other) const {
    return id <=> other.id;
}

usize ComponentLut::size() const {
    return _lut.size();
}

core::Span<ComponentLut::Entry> ComponentLut::lut() const {
    return _lut;
}

void ComponentLut::add_ref(EntityId id, UntypedComponentRef ref) {
    _lut.emplace_back(id, ref);
    std::sort(_lut.begin(), _lut.end());
}

void ComponentLut::remove_ref(EntityId id) {
    const auto it = std::lower_bound(_lut.begin(), _lut.end(), id, [](const Entry& entry, EntityId id) {
        return entry.id < id;
    });
    y_debug_assert(it != _lut.end() && it->id == id);
    _lut.erase(it);
}






static usize find_smallest_lut_index(core::Span<LutQuery> luts, usize entity_count) {
    usize smallest_index = 0;
    usize smallest_size = usize(-1);

    for(usize i = 0; i != luts.size(); ++i) {
        if(luts[i].exlude) {
            // Queries don't support the driver being exluded
            continue;
        }

        const usize raw_size = luts[i].lut ? luts[i].lut->size() : 0;
        y_debug_assert(raw_size <= entity_count);

        const usize size = luts[i].exlude ? (entity_count - raw_size) : raw_size;
        if(size < smallest_size) {
            smallest_size = size;
            smallest_index = i;
        }
    }

    return smallest_index;
}

template<bool DriverExcluded>
static QueryResult compute_query_internal(core::Span<LutQuery> luts, usize smallest_index) {
    y_always_assert(!DriverExcluded, "Fully negative queries are not supported");

    y_debug_assert(luts[smallest_index].exlude == DriverExcluded);

    core::Span<ComponentLut::Entry> driver;
    core::ScratchPad<std::pair<core::Span<ComponentLut::Entry>, bool>> iterators(luts.size() - 1);

    for(usize i = 0; i != luts.size(); ++i) {
        const core::Span<ComponentLut::Entry> lut = luts[i].lut->lut();
        y_debug_assert(std::is_sorted(lut.begin(), lut.end()));
        if(i == smallest_index) {
            driver = lut;
        } else {
            iterators[i < smallest_index ? i : (i - 1)] = {
                lut,
                luts[i].exlude,
            };
        }
    }


    QueryResult result;
    for(const auto& driver_it : driver) {
        const EntityId id = driver_it.id;

        bool matching = true;
        for(auto& it : iterators) {
            while(!it.first.is_empty() && it.first[0].id < id) {
                it.first = it.first.take(1);
            }

            if(it.first.is_empty()) {
                matching = it.second;
                break;
            }
            matching &= (id == it.first[0].id) != it.second;
        }

        if(matching) {
            result.ids << id;

            for(usize i = 0; i != smallest_index; ++i) {
                if(!iterators[i].second) {
                    result.refs << iterators[i].first[0].ref;
                }
            }

            if(!DriverExcluded) {
                result.refs << driver_it.ref;
            }

            for(usize i = smallest_index; i != iterators.size(); ++i) {
                if(!iterators[i].second) {
                    result.refs << iterators[i].first[0].ref;
                }
            }
        }
    }

    return result;
}

QueryResult World::compute_query(core::Span<LutQuery> luts, usize entity_count) {
    if(luts.is_empty()) {
        return {};
    }

    const usize smallest_index = find_smallest_lut_index(luts, entity_count);
    if(luts[smallest_index].exlude) {
        return compute_query_internal<true>(luts, smallest_index);
    } else {
        return compute_query_internal<false>(luts, smallest_index);
    }
}


}

