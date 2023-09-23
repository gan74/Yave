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
    return _entry_count;
}

core::Span<ComponentLut::Entry> ComponentLut::lut() const {
    return core::Span<Entry>(_lut.data(), _entry_count);
}

void ComponentLut::add_ref(EntityId id, UntypedComponentRef ref) {
    _lut.emplace_back(id, ref);
}

void ComponentLut::remove_ref(EntityId id) {
    _to_remove.emplace_back(id);
}


UncheckedComponentRef ComponentLut::find(EntityId id) const {
    const auto sorted_end = _lut.begin() + _entry_count;

    {
        const auto it = std::lower_bound(_lut.begin(), sorted_end, id, [](const Entry& entry, EntityId id) {
            return entry.id < id;
        });

        if(it != sorted_end) {
            return it->ref;
        }
    }

    {
        const auto it = std::find_if(sorted_end, _lut.end(), [=](const auto& entry) { return entry.id == id; });
        if(it != _lut.end()) {
            return it->ref;
        }
    }

    return {};
}


void ComponentLut::flush_additions() {
    y_profile();

    if(_entry_count != _lut.size()) {
        std::sort(_lut.begin(), _lut.end());
        _entry_count = _lut.size();
    }
}

void ComponentLut::flush_removals(core::Vector<UncheckedComponentRef>* removed) {
    y_profile();

    if(!_to_remove.is_empty()) {
        std::sort(_to_remove.begin(), _to_remove.end());
        y_debug_assert(std::unique(_to_remove.begin(), _to_remove.end()) == _to_remove.end());

        auto remove_it = _to_remove.begin();
        auto it = std::lower_bound(_lut.begin(), _lut.end(), *remove_it, [](const Entry& entry, EntityId id) {
            return entry.id < id;
        });

        auto out_it = it;
        while(it != _lut.end()) {
            if(remove_it != _to_remove.end() && it->id == *remove_it) {
                if(removed) {
                    removed->push_back(it->ref);
                }
                ++remove_it;
            } else {
                *out_it = std::move(*it);
                ++out_it;
            }
            ++it;
        }

        y_debug_assert(usize(it - out_it) == _to_remove.size());
        _entry_count = _lut.size() - _to_remove.size();
        while(_lut.size() != _entry_count) {
            _lut.pop();
        }

        _to_remove.make_empty();
    }
}







EntityId World::create_entity() {
    return _entities.create_entity();
}

void World::remove_entity(EntityId id) {
    _deferred_removals.entities << id;
}

void World::tick() {
    y_profile();

    for(auto& system : _systems) {
        flush_additions();
        system->tick();
    }

    flush();
}

void World::flush() {
    y_profile();

    flush_additions();

    clear_mutated();
    flush_removals();
    audit();
}

void World::flush_additions() {
    y_profile();

    for(auto& lut : _lookup) {
        lut.flush_additions();
    }
}

void World::flush_removals() {
    y_profile();

    {
        y_profile_zone("Removing entities");
        for(const EntityId id : _deferred_removals.entities) {
            for(auto& lut : _lookup) {
                const UncheckedComponentRef ref = lut.find(id);
                if(!ref.is_null()) {
                    lut.remove_ref(id);
                }
            }
        }
        _deferred_removals.entities.make_empty();
    }

    core::Vector<UncheckedComponentRef> removed;
    {
        y_profile_zone("Removing components");
        for(auto& lut : _lookup) {
            lut.flush_removals(&removed);
        }
    }

    {
        y_profile_zone("Clearning removed components");
        for(UncheckedComponentRef ref : removed) {
            ref.pool()->remove(ref);
        }
    }
}

void World::clear_mutated() {
    y_profile();

    for(auto& pool : _pools) {
        pool->clear_mutated();
    }
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

void World::audit() {
#ifdef Y_DEBUG
#endif
}


}

