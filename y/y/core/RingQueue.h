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
#ifndef Y_CORE_RINGQUEUE_H
#define Y_CORE_RINGQUEUE_H

#include <y/utils.h>
#include <memory>

namespace y {
namespace core {

template<typename Elem, typename Allocator = std::allocator<Elem>>
class RingQueue : NonCopyable, Allocator {
    using data_type = typename std::remove_const<Elem>::type;

    public:
        using value_type = Elem;
        using size_type = usize;

        using reference = value_type&;
        using const_reference = const value_type&;

        using pointer = value_type*;
        using const_pointer = const value_type*;

        RingQueue() = default;

        RingQueue(usize capacity) : 
                _data(Allocator::allocate(capacity)),
                _capacity(capacity) {
        }

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

        void push(const_reference elem) {
            ::new(_data + next_index()) data_type(elem);
            ++_size;
        }

        void push(value_type&& elem) {
            ::new(_data + next_index()) data_type(std::move(elem));
            ++_size;
        }

        template<typename... Args>
        reference emplace(Args&&... args) {
            const auto& ref = *(::new(_data + next_index()) data_type(y_fwd(args)...));
            ++_size;
            return ref;
        }

        value_type pop() {
            y_debug_assert(!is_empty());
            const data_type r = std::move(_data[_beg_index]);
            _data[_beg_index].~data_type();
            increment_begin();
            return r;
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

        const_reference operator[](usize i) const {
            y_debug_assert(i < size());
            return _data[wrap(i + _beg_index)];
        }

        reference operator[](usize i) {
            y_debug_assert(i < size());
            return _data[wrap(i + _beg_index)];
        }

        const_reference first() const {
            y_debug_assert(!is_empty());
            return _data[_beg_index];
        }

        reference first() {
            y_debug_assert(!is_empty());
            return _data[_beg_index];
        }

        const_reference last() const {
            y_debug_assert(!is_empty());
            return _data[last_index()];
        }

        reference last() {
            y_debug_assert(!is_empty());
            return _data[last_index()];
        }


        usize size() const {
            return _size;
        }

        usize capacity() const {
            return _capacity;
        }

        bool is_empty() const {
            return _size ==  0;
        }

        bool is_full() const {
            return size() == _capacity;
        }

    private:
        usize wrap(usize i) const {
            y_debug_assert(i < _capacity * 2);
            return i >= _capacity ? i - _capacity : i;
        }

        usize next_index() const {
            return wrap(_beg_index + _size);
        }

        usize last_index() const {
            usize end = next_index();
            return (end ? end : _capacity) - 1;
        }

        void increment_begin() {
            y_debug_assert(!is_empty());
            ++_beg_index;
            if(_beg_index >= _capacity) {
                _beg_index = 0;
            }
            --_size;
        }

        Owner<data_type*> _data = nullptr;
        usize _beg_index = 0;
        usize _size = 0;
        usize _capacity = 0;
};

}
}

#endif // Y_CORE_RINGQUEUE_H

