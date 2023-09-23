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
#ifndef YAVE_WORLD_WORLD_H
#define YAVE_WORLD_WORLD_H

#include "ComponentPool.h"
#include "Query.h"
#include "Entity.h"

#include <y/core/ScratchPad.h>

namespace yave {

class ComponentLut : NonCopyable {
    public:
        struct Entry {
            EntityId id;
            UncheckedComponentRef ref;

            std::strong_ordering operator<=>(const Entry& other) const {
                return id <=> other.id;
            }
        };

        usize size() const {
            return _lut.size();
        }

        void add_ref(EntityId id, UntypedComponentRef ref) {
            _lut.emplace_back(id, ref);
            std::sort(_lut.begin(), _lut.end());
        }

        void remove_ref(EntityId id) {
            const auto it = std::lower_bound(_lut.begin(), _lut.end(), id, [](const Entry& entry, EntityId id) {
                return entry.id < id;
            });
            y_debug_assert(it != _lut.end() && it->id == id);
            _lut.erase(it);
        }

        core::Span<Entry> lut() const {
            return _lut;
        }

    private:
        core::Vector<Entry> _lut;
};

struct LutQuery {
    const ComponentLut* lut = nullptr;
    const bool exlude = false;
    const ComponentType type;
};





class World {
    public:
        Entity& create_entity() {
            return _entities.create_entity();
        }

        const Entity& entity(EntityId id) const {
            return _entities[id];
        }


        template<typename T, typename... Args>
        ComponentRef<T> add(EntityId id, Args&&... args) {
            const ComponentRef<T> ref = create_container<T>().add(y_fwd(args)...);
            _entities.register_component(id, ref);
            create_lut<T>().add_ref(id, ref);
            return ref;
        }

        template<typename T>
        void remove(EntityId id) {
            const UntypedComponentRef ref = _entities.unregister_component(id, component_type<T>());
            y_debug_assert(!ref.to_typed<T>().is_stale());
            ref.pool()->remove(ref);
            create_lut<T>().remove_ref(id);
            y_debug_assert(ref.to_typed<T>().is_stale());
        }

        template<typename... Args>
        Query<Args...> query() {
            const std::array luts{lut_for_query<Args>()...};
            return compute_query(luts, _entities.entity_count());
        }

    private:
        template<typename T>
        ComponentPool<T>& create_container() {
            const ComponentTypeIndex index = component_type<T>().index();
            _pools.set_min_size(index + 1);

            auto& pool = _pools[index];
            if(!pool) {
                pool = std::make_unique<ComponentPool<T>>();
            }

            ComponentPool<T>* pool_ptr = dynamic_cast<ComponentPool<T>*>(pool.get());
            y_debug_assert(pool_ptr);
            return *pool_ptr;
        }

        template<typename T>
        ComponentLut& create_lut() {
            const ComponentTypeIndex index = component_type<T>().index();
            _lookup.set_min_size(index + 1);
            return _lookup[index];
        }

        template<typename T>
        LutQuery lut_for_query() {
            using traits = detail::query_traits<T>;
            const ComponentType type = component_type<typename traits::raw_type>();
            const ComponentTypeIndex type_index = type.index();
            if(type_index < _lookup.size()) {
                return LutQuery {
                    &_lookup[type_index],
                    traits::exclude,
                    type,
                };
            }
            return {};
        }

        static QueryResult compute_query(core::Span<LutQuery> luts, usize entity_count);

        core::Vector<std::unique_ptr<ComponentPoolBase>> _pools;
        core::Vector<ComponentLut> _lookup;

        EntityContainer _entities;
};

}


#endif // YAVE_WORLD_WORLD_H

