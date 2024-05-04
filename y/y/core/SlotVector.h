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
#ifndef Y_CORE_SLOTVECTOR_H
#define Y_CORE_SLOTVECTOR_H

#include "Vector.h"
#include "FixedArray.h"

#include <algorithm>
#include <numeric>

namespace y {
namespace core {

template<typename Elem, typename Allocator = std::allocator<Elem>>
class SlotVector : Allocator, NonCopyable {

    using data_type = typename std::remove_const<Elem>::type;

    using ResizePolicy = SmallVectorResizePolicy<16>;

    public:
        using value_type = Elem;
        using size_type = usize;

        using reference = value_type&;
        using const_reference = const value_type&;

        enum class Slot : u32 {
            invalid_slot = u32(-1),
        };

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

                Slot slot() const {
                    return Slot(*_it);
                }

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

                inline reference operator*() const {
                    return _data[*_it];
                }

                inline pointer operator->() const {
                    return &_data[*_it];
                }

                operator Iterator<true>() const {
                    return Iterator<true>(_data, _it);
                }

                inline std::strong_ordering operator<=>(const Iterator& other) const {
                    return _it <=> other._it;
                }

                bool operator==(const Iterator&) const = default;
                bool operator!=(const Iterator&) const = default;

            private:
                friend class SlotVector;
                friend class Iterator<!Const>;

                Iterator(data_type* data, const usize* it) : _data(data), _it(it) {
                }

                data_type* _data = nullptr;
                const usize* _it = nullptr;
        };

    public:
        using iterator = Iterator<false>;
        using const_iterator = Iterator<true>;

        SlotVector() = default;

        SlotVector(SlotVector&& other) {
            swap(other);
        }

        SlotVector& operator=(SlotVector&& other) {
            swap(other);
            return *this;
        }

        ~SlotVector() {
            clear();
        }



        void swap(SlotVector& other) {
            if(&other == this) {
                return;
            }

            std::swap(_data, other._data);
            std::swap(_size, other._size);
            _indices.swap(other._indices);

            if constexpr(std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value) {
                std::swap<Allocator>(*this, other);
            }
        }


        inline const_iterator begin() const {
            return const_iterator(_data, _indices.data());
        }

        inline const_iterator end() const {
            return const_iterator(_data, _indices.data() + _size);
        }

        inline iterator begin() {
            return iterator(_data, _indices.data());
        }

        inline iterator end() {
            return iterator(_data, _indices.data() + _size);
        }



        template<typename... Args>
        Slot insert(Args&&... args) {
            if(_size == _indices.size()) {
                expand();
            }

            const usize index = _indices[_size++];
            ::new(&_data[index]) data_type(y_fwd(args)...);

            return Slot(index);
        }

        void erase(iterator it) {
            const usize index = (it._it - _indices.data());
            y_debug_assert(index < _size);
            clear(_data[*it._it]);
            --_size;
            std::swap(_indices[index], _indices[_size]);
        }



        template<typename F>
        void sort(F&& compare) {
            std::sort(_indices.data(), _indices.data() + _size, [&](usize a, usize b) {
                return compare(operator[](Slot(a)), operator[](Slot(b)));
            });
        }

        void sort_indices() {
            std::sort(_indices.data(), _indices.data() + _size);
            std::sort(_indices.data() + _size, _indices.data() + _indices.size());
        }

        void make_empty() {
            for(usize i = 0; i != _size; ++i) {
                clear(_data[_indices[i]]);
            }
            _size = 0;
        }

        void clear() {
            make_empty();
            Allocator::deallocate(std::exchange(_data, nullptr), capacity());
            _indices = {};
        }

        void set_min_capacity(usize min_cap) {
            const usize new_cap = ResizePolicy::ideal_capacity(min_cap);
            if(new_cap > capacity()) {
                set_capacity(new_cap);
            }
        }


        inline reference operator[](Slot slot) {
            y_debug_assert(usize(slot) < capacity());
            return _data[usize(slot)];
        }

        inline const_reference operator[](Slot slot) const {
            y_debug_assert(usize(slot) < capacity());
            return _data[usize(slot)];
        }

        inline usize size() const {
            return _size;
        }

        inline bool is_empty() const {
            return !_size;
        }

        inline usize capacity() const {
            return _indices.size();
        }

    private:
        inline void clear(data_type& elem) {
            elem.~data_type();
#ifdef Y_DEBUG
            std::memset(&elem, 0xFE, sizeof(elem));
#endif
        }

        void expand() {
            set_capacity(ResizePolicy::ideal_capacity(capacity() + 1));
        }

        void set_capacity(usize new_capacity) {
            const usize prev_capacity = capacity();
            y_debug_assert(prev_capacity < new_capacity);

            data_type* new_data = Allocator::allocate(new_capacity);
            for(usize i = 0; i != _size; ++i) {
                const usize index = _indices[i];
                data_type& elem = _data[index];
                ::new(new_data + index) data_type(std::move(elem));
                clear(elem);
            }

            Allocator::deallocate(std::exchange(_data, new_data), prev_capacity);

            FixedArray<usize> new_indices(new_capacity);
            std::copy_n(_indices.begin(), prev_capacity, new_indices.begin());
            std::iota(new_indices.begin() + prev_capacity, new_indices.end(), prev_capacity);
            _indices = std::move(new_indices);

            y_debug_assert(capacity() == new_capacity);
        }

        data_type* _data = nullptr;
        FixedArray<usize> _indices;
        usize _size = 0;

};

}
}

#endif // Y_CORE_SLOTVECTOR_H

