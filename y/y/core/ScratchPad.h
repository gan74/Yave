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
#include <iterator>
#include <algorithm>

namespace y {
namespace core {

template<typename Elem>
class ScratchPadBase : NonMovable {
    protected:
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

        inline bool operator==(const ScratchPadBase<value_type>& other) const {
            return _size == other._size && std::equal(begin(), end(), other.begin(), other.end());
        }

        inline bool operator!=(const ScratchPadBase<value_type>& v) const {
            return !operator==(v);
        }

        inline bool is_null() const {
            return !_data;
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

    protected:
        inline ScratchPadBase() = default;

        inline ScratchPadBase(data_type* data, usize size) : _data(data), _size(size) {
        }

        inline void swap(ScratchPadBase& other) {
            std::swap(_data, other._data);
            std::swap(_size, other._size);
        }

        inline void clear() {
            for(usize i = 0; i != _size; ++i) {
                _data[i].~data_type();
            }
        }

        data_type* _data = nullptr;
        usize _size = 0;
};


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
class ScratchPad : public ScratchPadBase<Elem> {
    using data_type = typename ScratchPadBase<Elem>::data_type;

    public:
        using value_type = typename ScratchPadBase<Elem>::value_type;
        using size_type = typename ScratchPadBase<Elem>::size_type;

        using reference = typename ScratchPadBase<Elem>::reference;
        using const_reference = typename ScratchPadBase<Elem>::const_reference;

        using pointer = typename ScratchPadBase<Elem>::pointer;
        using const_pointer = typename ScratchPadBase<Elem>::const_pointer;

        using iterator = typename ScratchPadBase<Elem>::iterator;
        using const_iterator = typename ScratchPadBase<Elem>::const_iterator;

        inline ScratchPad() = default;

        inline ScratchPad(usize size) : ScratchPadBase<Elem>(detail::alloc_typed_scratchpad<data_type>(size), size) {
            for(usize i = 0; i != this->_size; ++i) {
                new(this->_data + i) data_type();
            }
        }

        inline explicit ScratchPad(Span<value_type> other) : ScratchPadBase<Elem>(detail::alloc_typed_scratchpad<data_type>(other.size()), other.size()) {
            for(usize i = 0; i != this->_size; ++i) {
                new(this->_data + i) data_type(other[i]);
            }
        }

        inline ~ScratchPad() {
            if(this->_data) {
                this->clear();
                detail::free_typed_scratchpad(this->_data, this->_size);
            }
        }

        ScratchPad(ScratchPad&& other) {
            this->swap(other);
        }

        ScratchPad& operator=(ScratchPad&& other) {
            this->swap(other);
            return *this;
        }

};

template<typename Elem>
class ScratchVector : public ScratchPadBase<Elem> {
    using data_type = typename ScratchPadBase<Elem>::data_type;

    public:
        using value_type = typename ScratchPadBase<Elem>::value_type;
        using size_type = typename ScratchPadBase<Elem>::size_type;

        using reference = typename ScratchPadBase<Elem>::reference;
        using const_reference = typename ScratchPadBase<Elem>::const_reference;

        using pointer = typename ScratchPadBase<Elem>::pointer;
        using const_pointer = typename ScratchPadBase<Elem>::const_pointer;

        using iterator = typename ScratchPadBase<Elem>::iterator;
        using const_iterator = typename ScratchPadBase<Elem>::const_iterator;

        inline ScratchVector() = default;

        inline ScratchVector(usize capacity) : ScratchPadBase<Elem>(detail::alloc_typed_scratchpad<data_type>(capacity), 0) {
            _capacity = capacity;
        }

        inline ScratchVector(usize size, const value_type& elem) : ScratchVector(size) {
            if(size) {
                std::fill_n(std::back_inserter(*this), size, elem);
            }
        }

        inline explicit ScratchVector(Span<value_type> other) : ScratchVector(other.size()) {
            for(const auto& e : other) {
                emplace_back(e);
            }
        }

        inline ~ScratchVector() {
            if(this->_data) {
                this->clear();
                detail::free_typed_scratchpad(this->_data, _capacity);
            }
        }

        inline ScratchVector(ScratchVector&& other) {
            this->swap(other);
        }

        inline ScratchVector& operator=(ScratchVector&& other) {
            this->swap(other);
            return *this;
        }

        inline bool is_full() const {
            return this->_size == _capacity;
        }

        inline bool is_empty() const {
            return !this->_size;
        }

        inline usize capacity() const {
            return _capacity;
        }

        inline reference push_back(const_reference elem) {
            y_debug_assert(!is_full());
            return *(::new(this->_data + this->_size++) data_type{elem});
        }

        inline reference push_back(value_type&& elem) {
            y_debug_assert(!is_full());
            return *(::new(this->_data + this->_size++) data_type{std::move(elem)});
        }

        template<typename... Args>
        inline reference emplace_back(Args&&... args) {
            y_debug_assert(!is_full());
            return *(::new(this->_data + this->_size++) data_type{y_fwd(args)...});
        }

        inline value_type pop() {
            y_debug_assert(!is_empty());
            data_type r = std::move(this->last());
            this->last().~data_type();
            --this->_size;

            return r;
        }

        inline void erase_unordered(iterator it) {
            if(it != this->end() - 1) {
                std::swap(*it, this->last());
            }
            pop();
        }

    private:
        inline void swap(ScratchVector& other) {
            ScratchPadBase<Elem>::swap(other);
            std::swap(_capacity, other._capacity);
        }

        usize _capacity = 0;
};



}
}


#endif

