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
#include <y/core/Range.h>

#include <y/utils/iter.h>

namespace yave {

class ComponentLut : NonCopyable {
    public:
        struct Entry {
            EntityId id;
            UncheckedComponentRef ref;

            std::strong_ordering operator<=>(const Entry& other) const;
        };

        usize size() const;
        core::Span<Entry> lut() const;

        void add_ref(EntityId id, UntypedComponentRef ref);
        void remove_ref(EntityId id);

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

        void clear_mutated() {
            for(auto& pool : _pools) {
                pool->clear_mutated();
            }
        }





        template<typename T, typename... Args>
        ComponentRef<T> add(EntityId id, Args&&... args) {
            const ComponentRef<T> ref = create_pool<T>().add(y_fwd(args)...);
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





        template<typename T>
        auto mutated() const {
            const ComponentPool<T>* pool = find_pool<T>();
            const core::Span<UncheckedComponentRef> mut = pool ? pool->_mutated : core::Span<UncheckedComponentRef>();
            return core::Range(
                TransformIterator(mut.begin(), [](UncheckedComponentRef ref) { return ref.to_typed_unchecked<T>(); }),
                mut.end()
            );
        }

        template<typename... Args>
        Query<Args...> query() {
            const std::array luts{lut_for_query<Args>()...};
            return compute_query(luts, _entities.entity_count());
        }


    private:
        template<typename T>
        ComponentPool<T>& create_pool() {
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
        const ComponentPool<T>* find_pool() const {
            const ComponentTypeIndex index = component_type<T>().index();
            if(_pools.size() <= usize(index)) {
                return nullptr;
            }

            return dynamic_cast<const ComponentPool<T>*>(_pools[index].get());
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

