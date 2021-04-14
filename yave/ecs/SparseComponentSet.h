/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include <tuple>

// #define YAVE_ECS_COMPONENT_SET_AUDIT


namespace yave {
namespace ecs {

class SparseIdSet : NonCopyable {

    public:
        using index_type = u32;
        using size_type = usize;

        static constexpr index_type invalid_index = index_type(-1);

        bool contains(EntityId id) const {
            return contains_index(id.index()) && _dense[_sparse[id.index()]] == id;
        }

        bool contains_index(index_type index) const {
            return index < _sparse.size() && _sparse[index] != invalid_index;
        }

        usize size() const {
            return _dense.size();
        }

        bool is_empty() const {
            return _dense.is_empty();
        }

        core::Span<EntityId> ids() const {
            return _dense;
        }

        index_type dense_index_of(EntityId id) const {
            return contains(id) ? _sparse[id.index()] : invalid_index;
        }

        index_type dense_index_of(index_type index) const {
            return index < _sparse.size() ? _sparse[index] : invalid_index;
        }

    protected:
        void grow_sparse(index_type max_index) {
            const usize target_size = usize(max_index + 1);
            _sparse.set_min_capacity(target_size);
            while(_sparse.size() < target_size) {
                _sparse << invalid_index;
            }
        }

        core::Vector<EntityId> _dense;
        core::Vector<index_type> _sparse;
};


template<typename Elem>
class SparseComponentSetBase : public SparseIdSet {

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

            public:
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

                inline bool operator==(const PairIterator& other) const {
                    return _id == other._id;
                }

                inline bool operator!=(const PairIterator& other) const {
                    return !operator==(other);
                }

            private:
                friend class SparseComponentSetBase;

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
            y_debug_assert(!contains(id));

            const index_type index = id.index();
            grow_sparse(index);

            _sparse[index] = index_type(_dense.size());
            _dense.emplace_back(id);
            _values.emplace_back(y_fwd(args)...);

            audit();

            return _values.last();
        }

        reference insert(value_type&& value) {
            return insert(std::get<0>(value), std::move(std::get<1>(value)));
        }

        void erase(EntityId id) {
            y_debug_assert(contains(id));

            const index_type index = id.index();
            const index_type dense_index = _sparse[index];
            const index_type last_dense_index = index_type(_dense.size() - 1);
            const EntityId last = _dense[last_dense_index];

            y_debug_assert(_dense[dense_index] == id);

            std::swap(_dense[dense_index], _dense[last_dense_index]);
            std::swap(_values[dense_index], _values[last_dense_index]);

            _dense.pop();
            _values.pop();

            const index_type last_sparse_index = last.index();
            _sparse[last_sparse_index] = dense_index;
            _sparse[index] = invalid_index;

            audit();

            y_debug_assert(!contains(id));
        }


        reference operator[](EntityId id) {
            y_debug_assert(contains(id));
            return _values[_sparse[id.index()]];
        }

        const_reference operator[](EntityId id) const {
            y_debug_assert(contains(id));
            return _values[_sparse[id.index()]];
        }

        pointer try_get(EntityId id) {
            const index_type index = id.index();
            if(index >= _sparse.size()) {
                return nullptr;
            }
            const index_type pi = _sparse[index];
            return pi < _values.size() ? &_values[pi] : nullptr;
        }

        const_pointer try_get(EntityId id) const {
            const index_type index = id.index();
            if(index >= _sparse.size()) {
                return nullptr;
            }
            const index_type pi = _sparse[index];
            return pi < _values.size() ? &_values[pi] : nullptr;
        }

        void set_min_capacity(usize cap) {
            _values.set_min_capacity(cap);
            _dense.set_min_capacity(cap);
        }

        void clear() {
            _values.clear();
            _dense.clear();
            _sparse.clear();
            audit();
        }


        void swap(SparseComponentSetBase& v) {
            if(&v != this) {
                _values.swap(v._values);
                _dense.swap(v._dense);
                _sparse.swap(v._sparse);
            }
            audit();
        }

        const_iterator begin() const {
            return const_iterator(_dense.begin(), _values.begin());
        }

        const_iterator end() const {
            return const_iterator(_dense.end(), _values.end());
        }

        const_iterator cbegin() const {
            return const_iterator(_dense.begin(), _values.begin());
        }

        const_iterator cend() const {
            return const_iterator(_dense.end(), _values.end());
        }

        iterator begin() {
            return iterator(_dense.begin(), _values.begin());
        }

        iterator end() {
            return iterator(_dense.end(), _values.end());
        }

        core::Range<iterator> as_pairs() {
            return {begin(), end()};
        }

        core::Range<const_iterator> as_pairs() const {
            return {begin(), end()};
        }

        core::MutableSpan<element_type> values() {
            return _values;
        }

        core::Span<element_type> values() const {
            return _values;
        }

    private:
        core::Vector<element_type> _values;

        void audit() {
#ifdef YAVE_ECS_COMPONENT_SET_AUDIT
            y_debug_assert(_dense.size() == _values.size());
            usize total = 0;
            for(usize i = 0; i != _sparse.size(); ++i) {
                for(usize o = 0; o != page_size; ++o) {
                    const page_index_type index = _sparse[i][o];
                    if(index != page_invalid_index) {
                        y_debug_assert(index < _dense.size());
                        const EntityId id = _dense[index];
                        y_debug_assert(id.is_valid());
                        y_debug_assert(page_index(id.index()) == std::pair(i, o));
                        ++total;
                    }
                }
            }
            y_debug_assert(total == _dense.size());
#endif
        }

};

// Why we need to do this to not have imcomplete types?
template<typename Elem>
class SparseComponentSet : public SparseComponentSetBase<Elem> {
};

}
}

#endif // YAVE_ECS_SPARSECOMPONENTSET_H

