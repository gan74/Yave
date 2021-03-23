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
#ifndef Y_CORE_VECTOR_H
#define Y_CORE_VECTOR_H

#include "Span.h"

#include <cstring>
#include <algorithm>
#include <iterator>

#ifdef Y_DEBUG
#define Y_VECTOR_ELECTRIC
#endif

#ifdef Y_VECTOR_ELECTRIC
#define Y_CLEAR_ELECTRIC(ptr, size) do { std::memset(static_cast<void*>(ptr), 0xFE, (size) * sizeof(data_type)); } while(false)
#define Y_CHECK_ELECTRIC(ptr, size) do { for(usize i = 0; i != (size) * sizeof(data_type); ++i) { y_debug_assert(static_cast<const u8*>(static_cast<const void*>(ptr))[i] == 0xFE); } } while(false)
#else
#define Y_CLEAR_ELECTRIC(ptr, size)
#define Y_CHECK_ELECTRIC(ptr, size)
#endif

namespace y {
namespace core {

struct DefaultVectorResizePolicy {
    static constexpr usize minimum = 16;
    static constexpr usize delta = 16;

    static usize ideal_capacity(usize size) {
        if(!size) {
            return 0;
        }
        if(size < minimum) {
            return minimum;
        }
        const usize l = log2ui(size + delta);
        return (4 << l) - (1 << l) - delta;
    }

    static bool shrink(usize size, usize capacity) {
        unused(size, capacity);
        return false;
        //return !size || (capacity - size) > 2 * step;
    }
};


template<typename Elem, typename ResizePolicy = DefaultVectorResizePolicy, typename Allocator = std::allocator<Elem>>
class Vector : ResizePolicy, Allocator {

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

        inline Vector() = default;

        inline explicit Vector(const Vector& other) : Vector(other.begin(), other.end()) {
        }

        inline Vector(usize size, const value_type& elem) {
            if(size) {
                set_min_capacity(size);
                std::fill_n(std::back_inserter(*this), size, elem);
            }
        }

        template<typename It>
        inline Vector(It beg_it, It end_it) {
            assign(beg_it, end_it);
        }

        inline Vector(Vector&& other) {
            swap(other);
        }

        inline Vector(std::initializer_list<value_type> other) : Vector(other.begin(), other.end()) {
        }

        inline explicit Vector(Span<value_type> other) : Vector(other.begin(), other.end()) {
        }

        template<typename... Args>
        inline Vector(const Vector<Elem, Args...>& other) : Vector(other.begin(), other.end()) {
        }

        inline Vector& operator=(Vector&& other) {
            swap(other);
            return *this;
        }

        inline Vector& operator=(const Vector& other) {
            if(&other != this) {
                if constexpr(std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value) {
                    clear();
                    Allocator::operator=(other);
                }
                assign(other.begin(), other.end());
            }
            return *this;
        }

        inline Vector& operator=(std::initializer_list<value_type> l) {
            assign(l.begin(), l.end());
            return *this;
        }

        inline Vector& operator=(Span<value_type> l) {
            assign(l.begin(), l.end());
            return *this;
        }

        template<typename... Args>
        inline Vector& operator=(const Vector<Elem, Args...>& l) {
            assign(l.begin(), l.end());
            return *this;
        }

        inline bool operator==(Span<value_type> v) const {
            return size() == v.size() ? std::equal(begin(), end(), v.begin(), v.end()) : false;
        }

        inline bool operator!=(Span<value_type> v) const {
            return !operator==(v);
        }

        template<typename... Args>
        inline bool operator==(const Vector<Elem, Args...>& v) const {
            return operator==(Span<value_type>(v));
        }

        template<typename... Args>
        inline bool operator!=(const Vector<Elem, Args...>& v) const {
            return operator!=(Span<value_type>(v));
        }


        template<typename It>
        inline void assign(It beg_it, It end_it) {
            if(contains_it(beg_it)) {
                Vector other(beg_it, end_it);
                swap(other);
            } else {
                make_empty();
                push_back(beg_it, end_it);
            }
        }

        inline void swap(Vector& v) {
            if(&v != this) {
                if constexpr(std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value) {
                    std::swap<Allocator>(*this, v);
                }
                std::swap(_data, v._data);
                std::swap(_data_end, v._data_end);
                std::swap(_alloc_end, v._alloc_end);
            }
        }

        inline ~Vector() {
            clear();
        }

        inline void push_back(const_reference elem) {
            if(is_full()) {
                expend();
            }

            Y_CHECK_ELECTRIC(_data_end, 1);

            ::new(_data_end++) data_type{elem};
        }

        inline void push_back(value_type&& elem) {
            if(is_full()) {
                expend();
            }

            Y_CHECK_ELECTRIC(_data_end, 1);

            ::new(_data_end++) data_type{std::move(elem)};
        }

        template<typename... Args>
        inline reference emplace_back(Args&&... args) {
            if(is_full()) {
                expend();
            }

            Y_CHECK_ELECTRIC(_data_end, 1);

            return *(::new(_data_end++) data_type{y_fwd(args)...});
        }

        template<typename It>
        inline void push_back(It beg_it, It end_it) {
            set_min_capacity(size() + std::distance(beg_it, end_it));
            std::copy(beg_it, end_it, std::back_inserter(*this));
        }

        inline value_type pop() {
            y_debug_assert(!is_empty());
            --_data_end;
            data_type r = std::move(*_data_end);
            _data_end->~data_type();

            Y_CLEAR_ELECTRIC(_data_end, 1);

            shrink();
            return r;
        }

        inline void erase_unordered(iterator it) {
            if(it != end() - 1) {
                std::swap(*it, last());
            }
            pop();
        }

        inline void erase(iterator it) {
            std::move(it + 1, end(), it);
            pop();
        }

        inline usize size() const {
            return _data_end - _data;
        }

        inline bool is_empty() const {
            return _data == _data_end;
        }

        inline usize capacity() const {
            return _alloc_end - _data;
        }

        inline const_iterator begin() const {
            return _data;
        }

        inline const_iterator end() const {
            return _data_end;
        }

        inline const_iterator cbegin() const {
            return _data;
        }

        inline const_iterator cend() const {
            return _data_end;
        }

        inline iterator begin() {
            return _data;
        }

        inline iterator end() {
            return _data_end;
        }

        inline pointer data() {
            return _data;
        }

        inline const_pointer data() const {
            return _data;
        }

        inline const_reference operator[](usize i) const {
            y_debug_assert(i < size());
            return _data[i];
        }

        inline reference operator[](usize i) {
            y_debug_assert(i < size());
            return _data[i];
        }

        inline const_reference first() const {
            y_debug_assert(!is_empty());
            return *_data;
        }

        inline reference first() {
            y_debug_assert(!is_empty());
            return *_data;
        }

        inline const_reference last() const {
            y_debug_assert(!is_empty());
            return *(_data_end - 1);
        }

        inline reference last() {
            y_debug_assert(!is_empty());
            return *(_data_end - 1);
        }

        inline void set_capacity(usize cap) {
            unsafe_set_capacity(cap);
        }

        inline void set_min_capacity(usize min_cap) {
            unsafe_set_capacity(ResizePolicy::ideal_capacity(min_cap));
        }

        inline void reserve(usize cap) {
            if(capacity() < cap) {
                set_capacity(cap);
            }
        }

        inline void clear() {
            unsafe_set_capacity(0);
        }

        inline void squeeze() {
            set_capacity(size());
        }

        inline void make_empty() {
            clear(_data, _data_end);
            _data_end = _data;
        }

    private:
        static constexpr bool is_data_trivial = std::is_trivial_v<data_type>;

        inline bool is_full() const {
            return _data_end == _alloc_end;
        }

        inline bool contains_it(const_iterator it) const {
            return it >= _data && it < _data_end;
        }

        inline void move_range(data_type* dst, data_type* src, usize n) {
            Y_CHECK_ELECTRIC(dst, n);
            if constexpr(is_data_trivial) {
                std::copy_n(src, n, dst);
            } else {
                for(usize i = 0; i != n; ++i) {
                    ::new(&dst[i]) data_type{std::move(src[i])};
                }
            }
        }

        inline void clear(data_type* beg, data_type* en) {
            if(!is_data_trivial) {
                for(data_type* e = en; e != beg;) {
                    (--e)->~data_type();
                }
            }

            Y_CLEAR_ELECTRIC(beg, en - beg);
        }

        inline void expend() {
            unsafe_set_capacity(this->ideal_capacity(size() + 1));
        }

        inline void shrink() {
            usize current = size();
            usize cap = capacity();
            if(current < cap && ResizePolicy::shrink(current, cap)) {
                unsafe_set_capacity(ResizePolicy::ideal_capacity(current));
            }
        }

        // uses data_end !!
        inline void unsafe_set_capacity(usize new_cap) {
            if(new_cap == capacity()) {
                return;
            }

            const usize current_size = size();
            const usize num_to_move = new_cap < current_size ? new_cap : current_size;

            data_type* new_data = new_cap ? Allocator::allocate(new_cap) : nullptr;

            Y_CLEAR_ELECTRIC(new_data, new_cap);
            Y_CHECK_ELECTRIC(new_data, new_cap);

            if(new_data != _data) {
                move_range(new_data, _data, num_to_move);
                clear(_data, _data_end);

                if(_data) {
                    Allocator::deallocate(_data, capacity());
                }
            }

            _data = new_data;
            _data_end = _data + num_to_move;
            _alloc_end = _data + new_cap;
        }

        Owner<data_type*> _data = nullptr;
        data_type* _data_end = nullptr;
        data_type* _alloc_end = nullptr;
};

template<typename T>
inline auto vector_with_capacity(usize cap) {
    auto vec = Vector<T>();
    vec.set_min_capacity(cap);
    return vec;
}




template<typename... Args, typename T>
inline Vector<Args...>& operator<<(Vector<Args...>& vec, T&& t) {
    vec.push_back(y_fwd(t));
    return vec;
}

template<typename... Args, typename T>
inline Vector<Args...>& operator+=(Vector<Args...>& vec, T&& t) {
    vec.push_back(y_fwd(t));
    return vec;
}

template<typename... Args, typename T>
inline Vector<Args...> operator+(Vector<Args...> vec, T&& t) {
    vec.push_back(y_fwd(t));
    return vec;
}

template<typename T, usize = 0, typename... Args>
using SmallVector = Vector<T, Args...>; // for now...

}
}

#undef Y_CLEAR_ELECTRIC
#undef Y_CHECK_ELECTRIC

#endif

