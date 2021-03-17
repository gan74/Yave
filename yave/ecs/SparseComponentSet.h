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

#include <y/utils/iter.h>

#include <tuple>

// #define YAVE_ECS_COMPONENT_SET_AUDIT

namespace yave {
namespace ecs {

class SparseIdSet : NonCopyable {
    public:
        using size_type = usize;

    protected:
        static constexpr usize page_size = 1024;

        using page_index_type = u32;
        static constexpr page_index_type page_invalid_index = page_index_type(-1);
        using page_type = std::array<page_index_type, page_size>;

    public:
        bool contains(EntityId id) const {
            const auto [i, o] = page_index(id);
            return i < _sparse.size() &&
                   _sparse[i][o] != page_invalid_index &&
                   _dense[_sparse[i][o]] == id;
        }

        bool contains_index(u32 index) const {
            const auto [i, o] = page_index(index);
            return i < _sparse.size() && _sparse[i][o] != page_invalid_index;
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

    protected:
        static std::pair<usize, usize> page_index(EntityId id) {
            return page_index(id.index());
        }

        static std::pair<usize, usize> page_index(u32 index) {
            const usize i = usize(index) / page_size;
            const usize o = usize(index) % page_size;
            return {i, o};
        }

        page_type& create_page(usize new_page_index) {
            Y_TODO(Fix this (alloc pages on the heap?))
            while(new_page_index >= _sparse.size()) {
                _sparse.emplace_back();
                auto& page = _sparse.last();
                std::fill(page.begin(), page.end(), page_invalid_index);
            }
            return _sparse[new_page_index];
        }

        core::Vector<EntityId> _dense;
        core::Vector<page_type> _sparse;
};


template<typename Elem>
class SparseComponentSetBase : public SparseIdSet {

    public:
        using element_type = Elem;
        using size_type = usize;

        using value_type = std::tuple<EntityId, element_type>;

        using reference = element_type&;
        using const_reference = const element_type&;

        using pointer = element_type*;
        using const_pointer = const element_type*;

    private:
        core::Vector<element_type> _values;

    public:
        template<bool Const>
        class PairIterator {
            public:
                using pointer_t = std::conditional_t<Const, const_pointer, pointer>;

                const EntityId* _id = nullptr;
                pointer_t _value = nullptr;

                PairIterator() = default;
                PairIterator(const EntityId* id, pointer_t value) : _id(id), _value(value) {
                }

                auto operator*() const {
                    return std::tie(*_id, *_value);
                }

                PairIterator operator++() {
                    ++_id;
                    ++_value;
                    return *this;
                }

                PairIterator operator++(int) {
                    const PairIterator it = *this;
                    operator++();
                    return it;
                }

                bool operator==(const PairIterator& other) const {
                    return _id == other._id;
                }

                bool operator!=(const PairIterator& other) const {
                    return !operator==(other);
                }
        };

        using iterator = PairIterator<false>;
        using const_iterator = PairIterator<true>;

        template<typename... Args>
        reference insert(EntityId id, Args&&... args) {
            y_debug_assert(!contains(id));

            const auto [i, o] = page_index(id);
            create_page(i)[o] = page_index_type(_dense.size());
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

            const auto [i, o] = page_index(id);
            const page_index_type dense_index = _sparse[i][o];
            const page_index_type last_index = page_index_type(_dense.size() - 1);
            const EntityId last_sparse = _dense[last_index];

            y_debug_assert(_dense[dense_index] == id);

            std::swap(_dense[dense_index], _dense[last_index]);
            std::swap(_values[dense_index], _values[last_index]);

            _dense.pop();
            _values.pop();

            const auto [li, lo] = page_index(last_sparse);
            _sparse[li][lo] = dense_index;
            _sparse[i][o] = page_invalid_index;

            audit();

            y_debug_assert(!contains(id));
        }


        reference operator[](EntityId id) {
            y_debug_assert(contains(id));
            const auto [i, o] = page_index(id);
            return _values[_sparse[i][o]];
        }

        const_reference operator[](EntityId id) const {
            y_debug_assert(contains(id));
            const auto [i, o] = page_index(id);
            return _values[_sparse[i][o]];
        }

        pointer try_get(EntityId id) {
            const auto [i, o] = page_index(id);
            if(i >= _sparse.size()) {
                return nullptr;
            }
            const usize pi = _sparse[i][o];
            return pi < _values.size() ? &_values[pi] : nullptr;
        }

        const_pointer try_get(EntityId id) const {
            const auto [i, o] = page_index(id);
            if(i >= _sparse.size()) {
                return nullptr;
            }
            const usize pi = _sparse[i][o];
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
    public:
        // using iterator       = decltype(std::declval<      SparseComponentSet<Elem>>().begin());
        // using const_iterator = decltype(std::declval<const SparseComponentSet<Elem>>().begin());
};

}
}

#endif // YAVE_ECS_SPARSECOMPONENTSET_H

