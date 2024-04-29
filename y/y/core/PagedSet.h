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
#ifndef Y_CORE_PAGEDSET_H
#define Y_CORE_PAGEDSET_H

#include "Vector.h"

#include <algorithm>

namespace y {
namespace core {

template<typename Elem, usize PageSize = 512, typename Allocator = std::allocator<Elem>>
class PagedSet : Allocator, NonCopyable {

    using data_type = typename std::remove_const<Elem>::type;

    public:
        static constexpr usize page_size = PageSize;

        using value_type = Elem;
        using size_type = usize;

        using reference = value_type&;
        using const_reference = const value_type&;


    private:
        template<bool Const>
        class Iterator {
            public:
                using value_type = std::conditional_t<Const, const Elem, Elem>;
                using size_type = usize;

                using reference = value_type&;
                using pointer = value_type*;

                using iterator_category = std::bidirectional_iterator_tag;
                using difference_type = std::ptrdiff_t;

                inline Iterator& operator++() {
                    ++_it;
                    return *this;
                }

                inline Iterator operator++(int) {
                    const Iterator it = *this;
                    ++_it;
                    return it;
                }

                inline Iterator& operator--() {
                    --_it;
                    return *this;
                }

                inline Iterator operator--(int) {
                    const Iterator it = *this;
                    --_it;
                    return it;
                }

                inline bool operator==(const Iterator& other) const {
                    return _it == other._it;
                }

                inline bool operator!=(const Iterator& other) const {
                    return _it != other._it;
                }

                inline reference operator*() const {
                    const usize index = *_it;
                    return _pages[index / page_size][index % page_size];
                }

                inline pointer operator->() const {
                    const usize index = *_it;
                    return _pages[index / page_size][index % page_size];
                }

                operator Iterator<true>() const {
                    return Iterator<true>(_pages, _it);
                }

            private:
                friend class PagedSet;
                friend class Iterator<!Const>;

                Iterator(data_type* const* pages, const usize* it) : _pages(pages), _it(it) {
                }

                data_type* const* _pages = nullptr;
                const usize* _it = nullptr;
        };

    public:
        using iterator = Iterator<false>;
        using const_iterator = Iterator<true>;

        PagedSet() = default;

        PagedSet(PagedSet&& other) {
            swap(other);
        }

        PagedSet& operator=(PagedSet&& other) {
            swap(other);
            return *this;
        }

        ~PagedSet() {
            clear();
        }



        void swap(PagedSet& other) {
            if(&other == this) {
                return;
            }

            _pages.swap(other._pages);
            _indices.swap(other._indices);
            std::swap(_size, other._size);

            if constexpr(std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value) {
                std::swap<Allocator>(*this, other);
            }
        }


        inline const_iterator begin() const {
            return const_iterator(_pages.data(), _indices.data());
        }

        inline const_iterator end() const {
            return const_iterator(_pages.data(), _indices.data() + _size);
        }

        inline iterator begin() {
            return iterator(_pages.data(), _indices.data());
        }

        inline iterator end() {
            return iterator(_pages.data(), _indices.data() + _size);
        }



        template<typename... Args>
        reference emplace(Args&&... args) {
            if(_size == _indices.size()) {
                add_page();
            }

            data_type* addr = get(_indices[_size++]);
            ::new(addr) data_type(y_fwd(args)...);

            return *addr;
        }

        void erase(iterator it) {
            const usize index = (it._it - _indices.data());
            y_debug_assert(index < _size);
            clear(get(*it._it));
            --_size;
            std::swap(_indices[index], _indices[_size]);
        }



        template<typename F>
        void sort(F&& compare) {
            std::sort(_indices.data(), _indices.data() + _size, [&](usize a, usize b) {
                return compare(operator[](a), operator[](b));
            });
        }

        void sort_indices() {
            std::sort(_indices.data(), _indices.data() + _size);
            std::sort(_indices.data() + _size, _indices.data() + _indices.size());
        }

        void make_empty() {
            for(usize i = 0; i != _size; ++i) {
                clear(get(_indices[i]));
            }
            _size = 0;
        }

        void clear() {
            make_empty();
            for(data_type* page : _pages) {
                Allocator::deallocate(page, page_size);
            }
            _pages.clear();
            _indices.clear();
        }



        inline usize size() const {
            return _size;
        }

        inline bool is_empty() const {
            return !_size;
        }

        inline reference operator[](usize index) {
            y_debug_assert(index < _size);
            return *get(index);
        }

        inline const_reference operator[](usize index) const {
            y_debug_assert(index < _size);
            return *get(index);
        }

    private:
        inline data_type* get(usize index) {
            return _pages[index / page_size] + (index % page_size);
        }

        inline const data_type* get(usize index) const {
            return _pages[index / page_size] + (index % page_size);
        }

        inline void clear(data_type* elem) {
            elem->~data_type();
#ifdef Y_DEBUG
            std::memset(elem, 0xFE, sizeof(*elem));
#endif
        }

        void add_page() {
            const usize page_count = _pages.size();
            _pages.emplace_back(Allocator::allocate(page_size));

            _indices.set_min_capacity((page_count + 1) * page_size);
            for(usize i = 0; i != page_size; ++i) {
                _indices.emplace_back(page_count * page_size + i);
            }

            y_debug_assert(_indices.size() == _pages.size() * page_size);
        }

        Vector<data_type*> _pages;
        Vector<usize> _indices;
        usize _size = 0;

};

}
}

#endif // Y_CORE_PAGEDSET_H

