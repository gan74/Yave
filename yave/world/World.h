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

#include "ComponentContainer.h"
#include "Query.h"
#include "Entity.h"

#include <y/core/ScratchPad.h>

namespace yave {

class ComponentLut : NonCopyable {
    public:
        struct Entry {
            EntityId id;
            UntypedComponentRef ref;

            bool operator<(const Entry& other) const {
                return id < other.id;
            }
        };

        usize size() const {
            return _lut.size();
        }

        void add_ref(EntityId id, UntypedComponentRef ref) {
            _lut.emplace_back(id, ref);
            std::sort(_lut.begin(), _lut.end());
        }

        core::Span<Entry> lut() const {
            return _lut;
        }

    private:
        core::Vector<Entry> _lut;
};

class World {
    public:
        template<typename T, typename... Args>
        ComponentRef<T> add(EntityId id, Args&&... args) {
            const ComponentRef<T> ref = create_container<T>().add(y_fwd(args)...);
            _entities.register_component(id, ref);
            create_lut<T>().add_ref(id, ref);
            return ref;
        }

        Entity& create_entity() {
            return _entities.create_entity();
        }

        const Entity& entity(EntityId id) const {
            return _entities[id];
        }


        template<typename... Args>
        Query<Args...> query() {
            std::array<ComponentLut*, sizeof...(Args)> luts = {};
            if(fill_luts<Args...>(luts)) {
                return compute_query(luts);
            }
            return {};
        }

    private:
        template<typename T>
        ComponentContainer<T>& create_container() {
            const ComponentTypeIndex index = component_type<T>().index();
            _containers.set_min_size(index + 1);

            auto& cont = _containers[index];
            if(!cont) {
                cont = std::make_unique<ComponentContainer<T>>();
            }

            ComponentContainer<T>* cont_ptr = dynamic_cast<ComponentContainer<T>*>(cont.get());
            y_debug_assert(cont_ptr);
            return *cont_ptr;
        }

        template<typename T>
        ComponentLut& create_lut() {
            const ComponentTypeIndex index = component_type<T>().index();
            _lookup.set_min_size(index + 1);
            return _lookup[index];
        }


        template<typename T, typename... Args>
        bool fill_luts(core::MutableSpan<ComponentLut*> luts) {
            y_debug_assert(luts.size() == sizeof...(Args) + 1);

            const ComponentTypeIndex type = component_type<T>().index();
            if(type < _lookup.size()) {
                luts[0] = &_lookup[type];
            } else {
                return false;
            }

            if constexpr(sizeof...(Args)) {
                return fill_luts<Args...>(luts.take(1));
            }

            return luts[0]->size();
        }

        static QueryResult compute_query(core::MutableSpan<ComponentLut*> luts);

        core::Vector<std::unique_ptr<ComponentContainerBase>> _containers;
        core::Vector<ComponentLut> _lookup;

        EntityContainer _entities;
};

}


#endif // YAVE_WORLD_WORLD_H

