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
#ifndef Y_CORE_FIXEDARRAY_H
#define Y_CORE_FIXEDARRAY_H

#include "Span.h"

#include <memory>

namespace y {
namespace core {

template<typename Elem>
class FixedArray {

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

        inline FixedArray() = default;

        inline FixedArray(usize size) : _data(std::make_unique<data_type[]>(size)), _size(size) {
        }

        inline FixedArray(FixedArray&&) = default;
        inline FixedArray& operator=(FixedArray&&) = default;


        inline bool operator==(const FixedArray<value_type>& v) const {
            return size() == v.size() ? std::equal(begin(), end(), v.begin(), v.end()) : false;
        }

        inline bool operator!=(const FixedArray<value_type>& v) const {
            return !operator==(v);
        }

        inline void swap(FixedArray& v) {
            if(&v != this) {
                std::swap(_data, v._data);
                std::swap(_size, v._size);
            }
        }

        inline void clear() {
            _size = 0;
            _data = nullptr;
        }

        inline void resize(usize s) {
            auto new_data = std::make_unique<data_type[]>(s);
            std::move(begin(), begin() + std::min(s, size()), new_data.get());

            _size = s;
            _data = std::move(new_data);
        }

        inline bool is_empty() const {
            return !_size;
        }

        inline usize size() const {
            return _size;
        }

        inline iterator begin() {
            return _data.get();
        }

        inline iterator end() {
            return _data.get() + _size;
        }

        inline const_iterator begin() const {
            return _data.get();
        }

        inline const_iterator end() const {
            return _data.get() + _size;
        }

        inline const_iterator cbegin() const {
            return _data.get();
        }

        inline const_iterator cend() const {
            return _data.get() + _size;
        }


        inline data_type* data() {
            return _data.get();
        }

        inline const data_type* data() const {
            return _data.get();
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
        std::unique_ptr<data_type[]> _data;
        usize _size = 0;


};

}
}


#endif // Y_CORE_FIXEDARRAY_H

