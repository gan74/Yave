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
#ifndef YAVE_ECS_SPARSECOMPONENTSET_H
#define YAVE_ECS_SPARSECOMPONENTSET_H

#include "ecs.h"

#include <y/core/Vector.h>
#include <y/core/Range.h>
#include <y/utils/traits.h>

#include <tuple>
#include <iterator>

// #define YAVE_ECS_COMPONENT_SET_AUDIT


namespace yave {
namespace ecs {

class SparseIdSetBase : NonMovable {
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
            u32 index = 0;
        };

        static_assert(sizeof(SparseElement) == sizeof(u64));

        inline void grow_sparse_if_needed(EntityId id) {
            _sparse.set_min_size(id.index() + 1);
        }

        inline std::pair<u32, u32> erase_id(EntityId id) {
            y_debug_assert(contains(id));

            SparseElement& elem = _sparse[id.index()];

            const u32 index = elem.index;
            const u32 last_index = u32(_ids.size() - 1);
            if(index == last_index) {
                _ids.pop();
            } else {
                const EntityId last = _ids[last_index];
                std::swap(_ids[index], _ids[last_index]);
                _ids.pop();
                y_debug_assert(_ids[index] == last);
                _sparse[last.index()].index = index;
            }

            elem = {};

            y_debug_assert(!contains(id));

            return {index, last_index};
        }

        inline SparseElement& insert_id(EntityId id) {
            y_debug_assert(id.is_valid());
            y_debug_assert(!contains(id));

            grow_sparse_if_needed(id);

            SparseElement& elem = _sparse[id.index()];

            elem.version = id.version();
            elem.index = u32(_ids.size());
            _ids << id;

            y_debug_assert(contains(id));

            return elem;
        }

        core::Vector<ecs::EntityId> _ids;
        core::Vector<SparseElement> _sparse;
};

class SparseIdSet final : public SparseIdSetBase {
    public:
        using iterator = decltype(_ids)::iterator;
        using const_iterator = decltype(_ids)::const_iterator;

        using value_type = EntityId;

        SparseIdSet() = default;

        SparseIdSet(SparseIdSet&& other) {
            swap(other);
        }

        SparseIdSet& operator=(SparseIdSet&& other) {
            swap(other);
            return *this;
        }

        void swap(SparseIdSet& other) {
            _ids.swap(other._ids);
            _sparse.swap(other._sparse);
        }

        inline bool insert(EntityId id) {
            if(contains(id)) {
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

        iterator begin() {
            return _ids.begin();
        }

        iterator end() {
            return _ids.end();
        }

        const_iterator begin() const {
            return _ids.begin();
        }

        const_iterator end() const {
            return _ids.end();
        }
};

static_assert(is_iterable_v<SparseIdSet>);


template<typename Elem>
class SparseComponentSet : public SparseIdSetBase {
    public:
        using element_type = std::remove_cv_t<Elem>;
        using size_type = usize;

        using value_type = std::tuple<EntityId, element_type>;

        using reference = element_type&;
        using const_reference = const element_type&;

        using pointer = element_type*;
        using const_pointer = const element_type*;

    private:
        template<bool Const>
        class PairIterator {
            using pointer_t = std::conditional_t<Const, const_pointer, pointer>;
            using reference_t = std::conditional_t<Const, const_reference, reference>;

            public:
                using difference_type = std::ptrdiff_t;
                using value_type = SparseComponentSet::value_type;
                using pointer = pointer_t;
                using reference = std::tuple<const EntityId&, reference_t>;
                using iterator_category = std::random_access_iterator_tag;

                inline PairIterator() = default;

                inline auto operator*() const {
                    return std::tie(*_id, *_value);
                }

                inline PairIterator operator++() {
                    ++_id;
                    ++_value;
                    return *this;
                }

                inline PairIterator operator++(int) {
                    const PairIterator it = *this;
                    operator++();
                    return it;
                }

                inline PairIterator& operator+=(difference_type index) {
                    _id += index;
                    _value += index;
                    return *this;
                }

                inline PairIterator operator+(difference_type index) const {
                    auto it = *this;
                    it += index;
                    return it;
                }

                inline PairIterator& operator-=(difference_type index) {
                    return operator+=(-index);
                }

                inline PairIterator operator-(difference_type index) const {
                    return operator+(-index);
                }

                inline difference_type operator-(const PairIterator& other) const {
                    y_debug_assert(_id - other._id == _value - other._value);
                    return _id - other._id;
                }

                inline bool operator==(const PairIterator& other) const {
                    return _id == other._id;
                }

                inline bool operator!=(const PairIterator& other) const {
                    return !operator==(other);
                }

            private:
                friend class SparseComponentSet;

                inline PairIterator(const EntityId* id, pointer_t value) : _id(id), _value(value) {
                }

                const EntityId* _id = nullptr;
                pointer_t _value = nullptr;
        };

    public:
        using iterator = PairIterator<false>;
        using const_iterator = PairIterator<true>;

        template<typename... Args>
        reference insert(EntityId id, Args&&... args) {
            insert_id(id);
            _values.emplace_back(y_fwd(args)...);

            return _values.last();
        }

        reference insert(value_type&& value) {
            return insert(std::get<0>(value), std::move(std::get<1>(value)));
        }

        bool erase(EntityId id) {
            if(!contains(id)) {
                return false;
            }

            const auto [a, b] = erase_id(id);
            if(a != b) {
                std::swap(_values[a], _values[b]);
            }
            _values.pop();

            y_debug_assert(!contains(id));

            return true;
        }


        reference operator[](EntityId id) {
            y_debug_assert(contains(id));
            return _values[_sparse[id.index()].index];
        }

        const_reference operator[](EntityId id) const {
            y_debug_assert(contains(id));
            return _values[_sparse[id.index()].index];
        }

        pointer try_get(EntityId id) {
            const u32 index = id.index();
            if(index >= _sparse.size()) {
                return nullptr;
            }
            const SparseElement elem = _sparse[index];
            y_debug_assert(elem.index < _values.size());
            return elem.version == id.version() ? &_values[elem.index] : nullptr;
        }

        const_pointer try_get(EntityId id) const {
            const u32 index = id.index();
            if(index >= _sparse.size()) {
                return nullptr;
            }
            const SparseElement elem = _sparse[index];
            y_debug_assert(elem.index < _values.size());
            return elem.version == id.version() ? &_values[elem.index] : nullptr;
        }

        void set_min_capacity(usize cap) {
            _ids.set_min_capacity(cap);
            _values.set_min_capacity(cap);
        }

        void make_empty() {
            _ids.make_empty();
            _sparse.make_empty();
            _values.make_empty();
        }

        void clear() {
            _ids.clear();
            _sparse.clear();
            _values.clear();
        }


        void swap(SparseComponentSet& v) {
            if(&v != this) {
                _ids.swap(v._ids);
                _sparse.swap(v._sparse);
                _values.swap(v._values);
            }
        }

        const_iterator begin() const {
            return const_iterator(_ids.begin(), _values.begin());
        }

        const_iterator end() const {
            return const_iterator(_ids.end(), _values.end());
        }

        iterator begin() {
            return iterator(_ids.begin(), _values.begin());
        }

        iterator end() {
            return iterator(_ids.end(), _values.end());
        }

        core::MutableSpan<element_type> values() {
            return _values;
        }

        core::Span<element_type> values() const {
            return _values;
        }

    private:
        core::Vector<element_type> _values;
};

}
}


#endif // YAVE_ECS_SPARSECOMPONENTSET_H

