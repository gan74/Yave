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
#ifndef Y_CORE_SCRATCHPAD_H
#define Y_CORE_SCRATCHPAD_H

#include "Span.h"

#include <memory>

namespace y {
namespace core {

namespace detail {
void* alloc_scratchpad(usize size);
void free_scratchpad(void* ptr, usize size);

template<typename T>
T* alloc_typed_scratchpad(usize size) {
    return static_cast<T*>(alloc_scratchpad(size * sizeof(T)));
}

template<typename T>
void free_typed_scratchpad(T* ptr, usize size) {
    free_scratchpad(ptr, size * sizeof(T));
}
}

template<typename Elem>
class ScratchPad : NonCopyable {

    using data_type = typename std::remove_const<Elem>::type;

    public:
        using value_type = Elem;
        using size_type = usize;

        using reference = value_type&;
        using const_reference = const value_type&;

        using pointer = value_type*;
        using const_pointer = const value_type*;

        using iterator = Elem*;
        using const_iterator = Elem const*;

        inline ScratchPad() = default;

        inline ScratchPad(usize size) : _data(detail::alloc_typed_scratchpad<data_type>(size)), _size(size) {
            for(usize i = 0; i != _size; ++i) {
                new(_data + i) data_type();
            }
        }

        inline ScratchPad(ScratchPad&& other) {
            swap(other);
        }

        inline ScratchPad& operator=(ScratchPad&& other) {
            swap(other);
            return this;
        }

        inline ~ScratchPad() {
            clear();
        }

        inline bool operator==(const ScratchPad<value_type>& v) const {
            return size() == v.size() ? std::equal(begin(), end(), v.begin(), v.end()) : false;
        }

        inline bool operator!=(const ScratchPad<value_type>& v) const {
            return !operator==(v);
        }

        inline void swap(ScratchPad& v) {
            if(&v != this) {
                std::swap(_data, v._data);
                std::swap(_size, v._size);
            }
        }

        inline void clear() {
            if(_data) {
                for(usize i = 0; i != _size; ++i) {
                    _data[i].~data_type();
                }
                detail::free_typed_scratchpad(_data, _size);
            }
        }

        inline bool is_empty() const {
            return !_size;
        }

        inline usize size() const {
            return _size;
        }

        inline iterator begin() {
            return _data;
        }

        inline iterator end() {
            return _data + _size;
        }

        inline const_iterator begin() const {
            return _data;
        }

        inline const_iterator end() const {
            return _data + _size;
        }

        inline const_iterator cbegin() const {
            return _data;
        }

        inline const_iterator cend() const {
            return _data + _size;
        }


        inline data_type* data() {
            return _data;
        }

        inline const data_type* data() const {
            return _data;
        }

        inline data_type& last() {
            return _data[_size - 1];
        }

        inline const data_type& last() const {
            return _data[_size - 1];
        }


        inline data_type& operator[](usize i) {
            y_debug_assert(i < _size);
            return _data[i];
        }

        inline const data_type& operator[](usize i) const {
            y_debug_assert(i < _size);
            return _data[i];
        }

    private:
        data_type* _data = nullptr;
        usize _size = 0;
};

}
}


#endif

