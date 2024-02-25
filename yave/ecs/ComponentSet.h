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

namespace yave {
namespace ecs {

class ComponentSetBase : NonMovable {
    public:
        inline bool contains(EntityId id) const {
            if(id.index() >= _sparse.size()) {
                return false;
            }

            const SparseElement& elem = _sparse[id.index()];
            return elem.version == id.version();
        }

        inline bool is_empty() const {
            return _ids.is_empty();
        }

        inline usize size() const {
            return _ids.size();
        }

        inline core::Span<EntityId> ids() const {
            return _ids;
        }

    protected:
        struct SparseElement {
            u32 version = 0;
            u32 id_index = u32(-1);
            void* ptr = nullptr;
        };

        inline void grow_sparse_if_needed(EntityId id) {
            _sparse.set_min_size(id.index() + 1);
        }

        inline void erase_id(EntityId id) {
            y_debug_assert(contains(id));

            SparseElement& elem = _sparse[id.index()];
            _ids.erase_unordered(_ids.begin() + elem.id_index);

            const EntityId new_id = _ids[elem.id_index];
            _sparse[new_id.index()].id_index = id.index();

            elem = {};

            y_debug_assert(std::count(_ids.begin(), _ids.end(), new_id) == 1);
            y_debug_assert(std::count(_ids.begin(), _ids.end(), id) == 0);

            y_debug_assert(!contains(id));
        }

        inline SparseElement& insert_id(EntityId id) {
            y_debug_assert(!contains(id));

            grow_sparse_if_needed(id);

            SparseElement& elem = _sparse[id.index()];
            y_debug_assert(!elem.ptr);

            elem.version = id.version();
            elem.id_index = u32(_ids.size());
            _ids << id;

            y_debug_assert(contains(id));

            return elem;
        }

        core::Vector<ecs::EntityId> _ids;
        core::Vector<SparseElement> _sparse;
};

class ComponentIdSet final : public ComponentSetBase {
    public:
        ComponentIdSet() = default;

        ComponentIdSet(ComponentIdSet&& other) {
            swap(other);
        }

        ComponentIdSet& operator=(ComponentIdSet&& other) {
            swap(other);
            return *this;
        }

        void swap(ComponentIdSet& other) {
            _ids.swap(other._ids);
            _sparse.swap(other._sparse);
        }

        inline bool insert(EntityId id) {
            if(!contains(id)) {
                return false;
            }
            insert_id(id);
            return true;
        }

        inline bool erase(EntityId id) {
            if(!contains(id)) {
                return false;
            }

            erase_id(id);

            return true;
        }

        void make_empty() {
            _ids.make_empty();
            _sparse.make_empty();
        }

        void clear() {
            _ids.make_empty();
            _sparse.clear();
        }
};

template<typename Elem>
class ComponentSet final : public ComponentSetBase {

    public:
        struct IdComponent {
            EntityId id;
            Elem component = {};

            IdComponent() = default;

            template<typename... Args>
            IdComponent(EntityId i, Args&&... args) : id(i), component(y_fwd(args)...) {
            }
        };

        using value_type = IdComponent;

        using pointer = value_type*;
        using const_pointer = const value_type*;

        using reference = value_type&;
        using const_reference = const value_type&;

        using iterator = core::PagedSet<IdComponent>::iterator;
        using const_iterator = core::PagedSet<IdComponent>::const_iterator;


        ComponentSet() = default;

        ComponentSet(ComponentSet&& other) {
            swap(other);
        }

        ComponentSet& operator=(ComponentSet&& other) {
            swap(other);
            return *this;
        }


        void swap(ComponentSet& other) {
            _ids.swap(other._ids);
            _sparse.swap(other._sparse);
            _components.swap(other._components);
        }

        inline Elem& insert(value_type&& value) {
            return insert(value.id, std::move(value.component));
        }

        template<typename... Args>
        inline Elem& insert(EntityId id, Args&&... args) {
            SparseElement& elem = insert_id(id);

            IdComponent& id_comp = _components.emplace(id, y_fwd(args)...);
            elem.ptr = &id_comp;

            y_debug_assert(_components.size() == _ids.size());

            return id_comp.component;
        }

        inline bool erase(EntityId id) {
            if(!contains(id)) {
                return false;
            }

            erase_id(id);

            const auto it = std::find_if(_components.begin(), _components.end(), [id](const IdComponent& id_comp) {
                return id_comp.id == id;
            });

            y_debug_assert(it != _components.end());
            _components.erase(it);

            y_debug_assert(_components.size() == _ids.size());

            return true;
        }

        void make_empty() {
            _ids.make_empty();
            _sparse.make_empty();
            _components.make_empty();
            y_debug_assert(is_empty());
        }

        void clear() {
            _ids.make_empty();
            _sparse.clear();
            _components.clear();
        }

        inline Elem* try_get(EntityId id) {
            if(id.index() >= _sparse.size()) {
                return nullptr;
            }

            SparseElement& elem = _sparse[id.index()];
            return elem.version == id.version() ? &static_cast<IdComponent*>(elem.ptr)->component : nullptr;
        }

        inline const Elem* try_get(EntityId id) const {
            if(id.index() >= _sparse.size()) {
                return nullptr;
            }

            const SparseElement& elem = _sparse[id.index()];
            return elem.version == id.version() ? &static_cast<const IdComponent*>(elem.ptr)->component : nullptr;
        }

        inline iterator begin() {
            return _components.begin();
        }

        inline iterator end() {
            return _components.end();
        }

        inline const_iterator begin() const {
            return _components.begin();
        }

        inline const_iterator end() const {
            return _components.end();
        }


        inline Elem& operator[](EntityId id) {
            y_debug_assert(contains(id));
            return *try_get(id);
        }

        inline const Elem& operator[](EntityId id) const {
            y_debug_assert(contains(id));
            return *try_get(id);
        }

    private:
        core::PagedSet<IdComponent> _components;
};

}
}


#endif // YAVE_ECS_COMPONENTSET_H

