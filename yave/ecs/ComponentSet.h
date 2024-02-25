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
#ifndef YAVE_ECS_COMPONENTSET_H
#define YAVE_ECS_COMPONENTSET_H

#include "ecs.h"

#include <y/core/PagedSet.h>

#include <tuple>

namespace yave {
namespace ecs {

template<typename Elem>
class ComponentSet : NonCopyable {

    struct IdComponent;

    struct SparseElement {
        u32 version = 0;
        IdComponent* ptr = nullptr;
    };

    public:
        struct IdComponent {
            EntityId id;
            Elem component;

            template<typename... Args>
            IdComponent(EntityId i, Args&&... args) : id(i), component(y_fwd(args)...) {
            }
        };

        using pointer = IdComponent*;
        using const_pointer = const IdComponent*;

        using reference = IdComponent&;
        using const_reference = const IdComponent&;

        using iterator = core::PagedSet<IdComponent>::iterator;
        using const_iterator = core::PagedSet<IdComponent>::iterator;



        ComponentSet() = default;

        ComponentSet(ComponentSet&& other) {
            swap(other);
        }

        ComponentSet& operator=(ComponentSet&& other) {
            swap(other);
            return *this;
        }


        void swap(ComponentSet& other) {
            _components.swap(other._components);
            _sparse.swap(other._sparse);
        }


        template<typename... Args>
        reference insert(EntityId id, Args&&... args) {
            y_debug_assert(!contains(id));
            grow_sparse_if_needed(id);

            SparseElement& elem = _sparse[id.index()];
            y_debug_assert(!elem.ptr);

            IdComponent& id_comp = _components.emplace(id, y_fwd(args)...);

            elem.version = id.version();
            elem.ptr = &id_comp;

            return id_comp;
        }

        bool erase(EntityId id) {
            if(!contains(id)) {
                return false;
            }

            const auto it = std::find(_components.begin(), _components.end(), [=](const IdComponent& id_comp) {
                return id_comp.id == id;
            });

            y_debug_assert(it != _components.end());
            _components.erase(it);
            _sparse[id.index()] = {};

            return true;
        }

        pointer try_get(EntityId id) {
            if(id.index() >= _sparse.size()) {
                return nullptr;
            }

            SparseElement& elem = _sparse[id.index()];
            return elem.version == id.version() ? elem.ptr : nullptr;
        }

        const_pointer try_get(EntityId id) const {
            if(id.index() >= _sparse.size()) {
                return nullptr;
            }

            const SparseElement& elem = _sparse[id.index()];
            return elem.version == id.version() ? elem.ptr : nullptr;
        }


        void make_empty() {
            _components.make_empty();
            _sparse.make_empty();
        }

        void clear() {
            _components.clear();
            _sparse.clear();
        }


        iterator begin() {
            return _components.begin();
        }

        iterator end() {
            return _components.end();
        }

        const_iterator begin() const {
            return _components.begin();
        }

        const_iterator end() const {
            return _components.end();
        }


        bool contains(EntityId id) const {
            return try_get(id);
        }

        usize size() const {
            return _components.size();
        }

    private:
        void grow_sparse_if_needed(EntityId id) {
            _sparse.set_min_size(id.index() + 1);
        }

        core::PagedSet<IdComponent> _components;
        core::Vector<SparseElement> _sparse;
};

}
}


#endif // YAVE_ECS_COMPONENTSET_H

