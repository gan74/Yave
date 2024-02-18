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
#ifndef Y_CORE_RINGQUEUE_H
#define Y_CORE_RINGQUEUE_H

#include "Vector.h"

namespace y {
namespace core {

template<typename Elem, typename Allocator = std::allocator<Elem>>
class RingQueue : NonCopyable, Allocator {
    using data_type = typename std::remove_const<Elem>::type;

    using ResizePolicy = DefaultVectorResizePolicy;

    template<bool Const>
    class Iterator {
        public:
            using value_type = std::conditional_t<Const, const Elem, Elem>;
            using size_type = usize;

            using reference = value_type&;
            using pointer = value_type*;

            using iterator_category = std::bidirectional_iterator_tag;
            using difference_type = std::ptrdiff_t;

            Iterator() = default;

            inline Iterator& operator++() {
                ++_index;
                return *this;
            }

            inline Iterator operator++(int) {
                const Iterator it = *this;
                ++_index;
                return it;
            }

            inline Iterator& operator--() {
                --_index;
                return *this;
            }

            inline Iterator operator--(int) {
                const Iterator it = *this;
                --_index;
                return it;
            }

            inline bool operator==(const Iterator& other) const {
                return _index == other._index;
            }

            inline bool operator!=(const Iterator& other) const {
                return _index != other._index;
            }

            inline reference operator*() const {
                y_debug_assert(_parent);
                return _parent->operator[](_index);
            }

            inline pointer operator->() const {
                y_debug_assert(_parent);
                return &_parent->operator[](_index);
            }

            operator Iterator<true>() const {
                return Iterator<true>(_parent, _index);
            }

        private:
            friend class RingQueue;
            friend class Iterator<!Const>;

            using parent_t = std::conditional_t<Const, const RingQueue, RingQueue>;

            Iterator(parent_t* parent, usize index) : _parent(parent), _index(index) {
            }

            parent_t* _parent = nullptr;
            usize _index = 0;
    };

    public:
        using value_type = Elem;
        using size_type = usize;

        using reference = value_type&;
        using const_reference = const value_type&;

        using pointer = value_type*;
        using const_pointer = const value_type*;

        using iterator = Iterator<false>;
        using const_iterator = Iterator<true>;


        RingQueue() = default;

        RingQueue(const RingQueue& other) = delete;
        RingQueue& operator=(const RingQueue& other) = delete;

        RingQueue(RingQueue&& other) {
            swap(other);
        }

        RingQueue& operator=(RingQueue&& other) {
            swap(other);
            return *this;
        }

        ~RingQueue() {
            make_empty();
            if(_data) {
                Allocator::deallocate(_data, capacity());
            }
        }


        void make_empty() {
            for(usize i = 0; i != _size; ++i) {
                operator[](i).~data_type();
            }
            _beg_index = 0;
            _size = 0;
        }

        void swap(RingQueue& v) {
            if(&v != this) {
                if constexpr(std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value) {
                    std::swap<Allocator>(*this, v);
                }
                std::swap(_data, v._data);
                std::swap(_beg_index, v._beg_index);
                std::swap(_size, v._size);
                std::swap(_capacity, v._capacity);
            }
        }

        inline void push_back(const_reference elem) {
            if(is_full()) {
                expand();
            }
            ::new(_data + next_index()) data_type(elem);
            ++_size;
        }

        inline void push_back(value_type&& elem) {
            if(is_full()) {
                expand();
            }
            ::new(_data + next_index()) data_type(std::move(elem));
            ++_size;
        }

        template<typename... Args>
        inline reference emplace_back(Args&&... args) {
            if(is_full()) {
                expand();
            }
            auto& ref = *(::new(_data + next_index()) data_type(y_fwd(args)...));
            ++_size;
            return ref;
        }

        template<typename... Args>
        inline void insert(const_iterator it, Args&&... args) {
            const usize index = it._index;
            if(index == size()) {
                emplace_back(y_fwd(args)...);
                return;
            }

            if(is_full()) {
                expand();
            }

            usize i = size() - 1;
            emplace_back(std::move(operator[](i)));
            while(i > index) {
                 operator[](i) = std::move(operator[](i - 1));
                 --i;
            }

            operator[](index) = data_type{y_fwd(args)...};
        }

        inline value_type pop_back() {
            y_debug_assert(!is_empty());
            const usize index = last_index();
            data_type r = std::move(_data[index]);
            _data[index].~data_type();
            --_size;
            return r;
        }

        inline value_type pop_front() {
            y_debug_assert(!is_empty());
            data_type r = std::move(_data[_beg_index]);
            _data[_beg_index].~data_type();
            increment_begin();
            return r;
        }

        inline const_reference operator[](usize i) const {
            y_debug_assert(i < size());
            return _data[wrap(i + _beg_index)];
        }

        inline reference operator[](usize i) {
            y_debug_assert(i < size());
            return _data[wrap(i + _beg_index)];
        }

        inline const_reference first() const {
            y_debug_assert(!is_empty());
            return _data[_beg_index];
        }

        inline reference first() {
            y_debug_assert(!is_empty());
            return _data[_beg_index];
        }

        inline const_reference last() const {
            y_debug_assert(!is_empty());
            return _data[last_index()];
        }

        inline reference last() {
            y_debug_assert(!is_empty());
            return _data[last_index()];
        }

        inline const_iterator begin() const {
            return const_iterator(this, 0);
        }

        inline const_iterator end() const {
            return const_iterator(this, size());
        }

        inline const_iterator cbegin() const {
            return begin();
        }

        inline const_iterator cend() const {
            return end();
        }

        inline iterator begin() {
            return iterator(this, 0);
        }

        inline iterator end() {
            return iterator(this, size());
        }


        inline usize size() const {
            return _size;
        }

        inline usize capacity() const {
            return _capacity;
        }

        inline bool is_empty() const {
            return _size ==  0;
        }

        inline void set_min_capacity(usize min_cap) {
            if(capacity() < min_cap) {
                unsafe_set_capacity(ResizePolicy::ideal_capacity(min_cap));
            }
        }

    private:
        inline bool is_full() const {
            return _size == _capacity;
        }

        inline usize wrap(usize i) const {
            y_debug_assert(_capacity == 0 || i < _capacity * 2);
            return i >= _capacity ? i - _capacity : i;
        }

        inline usize next_index() const {
            return wrap(_beg_index + _size);
        }

        inline usize last_index() const {
            return wrap(_beg_index + _size - 1);
        }

        inline void increment_begin() {
            y_debug_assert(!is_empty());
            ++_beg_index;
            if(_beg_index >= _capacity) {
                _beg_index = 0;
            }
            --_size;
        }

        inline void expand() {
            unsafe_set_capacity(ResizePolicy::ideal_capacity(size() + 1));
        }

        inline void unsafe_set_capacity(usize new_cap) {
            if(new_cap <= capacity()) {
                return;
            }

            y_debug_assert(new_cap > _size);
            data_type* new_data = Allocator::allocate(new_cap);

            if(_data) {
                for(usize i = 0; i != _size; ++i) {
                    auto& elem = operator[](i);
                    new(new_data + i) data_type(std::move(elem));
                    elem.~data_type();
                }

                Allocator::deallocate(_data, capacity());
            }

            _data = new_data;
            _capacity = new_cap;
            _beg_index = 0;
        }

        Owner<data_type*> _data = nullptr;
        usize _beg_index = 0;
        usize _size = 0;
        usize _capacity = 0;
};

}
}

#endif // Y_CORE_RINGQUEUE_H

