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
#ifndef Y_CORE_VECTOR_H
#define Y_CORE_VECTOR_H

#include "Span.h"

#include <cstring>
#include <algorithm>
#include <iterator>


namespace y {
namespace core {
namespace detail {
template<typename Elem, usize N>
struct SBOStorage {
    using data_type = typename std::remove_const<Elem>::type;
    union Storage {
        Storage() {}
        ~Storage() {}

        data_type storage[N];
    } sbo_storage;

    inline data_type* sbo_buffer() {
        return &sbo_storage.storage[0];
    }

    static_assert(sizeof(Storage) == N * sizeof(data_type));
};

template<typename Elem>
struct SBOStorage<Elem, 0> {
    using data_type = typename std::remove_const<Elem>::type;
    inline data_type* sbo_buffer() {
        return nullptr;
    }
};
}


template<usize Minimum>
struct SmallVectorResizePolicy {
    static_assert(Minimum > 0);

    static constexpr usize minimum = Minimum;
    static constexpr usize delta = 16;
    static constexpr usize offset_to_pow_2 = 8;

    // Start around x3.5, asymptotically goes to x2
    // For minimum of 16 (default): 16, 56, 120, 248, 504, 1016 , 2040, 4088, ...,  2^n-8
    static usize ideal_capacity(usize size) {
        if(!size) {
            return 0;
        }
        if(size <= minimum) {
            return minimum;
        }
        const usize l = log2ui(size + delta);
        return (2_uu << l) - offset_to_pow_2;
    }

    static bool shrink(usize size, usize capacity) {
        unused(size, capacity);
        return false;
        //return !size || (capacity - size) > 2 * step;
    }
};

using DefaultVectorResizePolicy = SmallVectorResizePolicy<16>;

template<typename Elem, typename Allocator = std::allocator<Elem>, typename SBOCapacity = std::integral_constant<usize, 0>>
class Vector : Allocator, detail::SBOStorage<Elem, SBOCapacity::value> {

    static constexpr bool has_sbo = SBOCapacity::value > 0;
    using data_type = typename std::remove_const<Elem>::type;
    using ResizePolicy = std::conditional_t<has_sbo, SmallVectorResizePolicy<SBOCapacity::value>, DefaultVectorResizePolicy>;

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

        inline explicit Vector(Span<value_type> other) : Vector(other.begin(), other.end()) {
        }

        inline Vector(std::initializer_list<value_type> other) : Vector(other.begin(), other.end()) {
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

        inline Vector& operator=(Span<value_type> other) {
            assign(other.begin(), other.end());
            return *this;
        }

        inline Vector& operator=(std::initializer_list<value_type> other) {
            assign(other.begin(), other.end());
            return *this;
        }


        inline bool operator==(Span<value_type> other) const {
            return size() == other.size() && std::equal(begin(), end(), other.begin(), other.end());
        }

        inline bool operator!=(Span<value_type> other) const {
            return !operator==(other);
        }

        template<typename... Args>
        inline bool operator==(const Vector<Elem, Args...>& other) const {
            return operator==(Span<value_type>(other));
        }

        template<typename... Args>
        inline bool operator!=(const Vector<Elem, Args...>& other) const {
            return operator!=(Span<value_type>(other));
        }


        template<typename It>
        inline void assign(It beg_it, It end_it) {
            if constexpr(std::is_pointer_v<It>) {
                y_debug_assert(!contains_it(beg_it));
            }

            make_empty();
            push_back(beg_it, end_it);
        }

        inline ~Vector() {
            clear();
        }

        inline void push_back(const_reference elem) {
            if(is_full()) {
                expand();
            }

            ::new(_data_end++) data_type{elem};
        }

        inline reference push_back(value_type&& elem) {
            if(is_full()) {
                expand();
            }

            return *(::new(_data_end++) data_type{std::move(elem)});
        }

        template<typename... Args>
        inline reference emplace_back(Args&&... args) {
            if(is_full()) {
                expand();
            }

            return *(::new(_data_end++) data_type{y_fwd(args)...});
        }

        template<typename It>
        inline void push_back(It beg_it, It end_it) {
            set_min_capacity(size() + std::distance(beg_it, end_it));
            std::copy(beg_it, end_it, std::back_inserter(*this));
        }

        template<typename... Args>
        inline void insert(const_iterator it, Args&&... args) {
            const usize index = it - _data;
            if(index == size()) {
                emplace_back(y_fwd(args)...);
                return;
            }

            if(is_full()) {
                expand();
            }

            usize i = size() - 1;
            ::new(_data_end++) data_type{std::move(_data[i])};
            while(i > index) {
                 _data[i] = std::move(_data[i - 1]);
                 --i;
            }

            _data[index] = data_type{y_fwd(args)...};
        }

        inline value_type pop() {
            y_debug_assert(!is_empty());
            --_data_end;
            data_type r = std::move(*_data_end);
            _data_end->~data_type();

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
            if(capacity() < min_cap) {
                unsafe_set_capacity(ResizePolicy::ideal_capacity(min_cap));
            }
        }

        inline void reserve(usize cap) {
            if(capacity() < cap) {
                set_capacity(cap);
            }
        }

        template<typename... Args>
        inline void set_min_size(usize min_size, Args&&... args) {
            if(min_size > size()) {
                set_min_capacity(min_size);
                while(size() < min_size) {
                    emplace_back(y_fwd(args)...);
                }
            }
        }

        inline void shrink_to(usize max_size) {
            if(size() > max_size) {
                data_type* end = _data + max_size;
                clear(end, _data_end);
                _data_end = end;
                y_debug_assert(size() == max_size);
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

        static inline Vector with_capacity(usize cap) {
            Vector v;
            v.set_min_capacity(cap);
            return v;
        }

        template<typename R>
        static inline Vector from_range(const R& range) {
            Vector v;
            for(const auto& e : range) {
                v.emplace_back(e);
            }
            return v;
        }

        static inline constexpr usize max_size() {
            return usize(-1);
        }

        inline void swap(Vector& other) {
            if(&other == this) {
                return;
            }

            if(!sbo_swap(other)) {
                std::swap(_data, other._data);
                std::swap(_data_end, other._data_end);
                std::swap(_alloc_end, other._alloc_end);
            }

            if constexpr(std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value) {
                std::swap<Allocator>(*this, other);
            }
        }

    private:
        inline bool sbo_swap(Vector& other) {
            if constexpr(has_sbo) {
                if(is_sbo_active() && other.is_sbo_active()) {
                    const usize this_size = size();
                    const usize other_size = other.size();
                    Vector* small = this_size < other_size ? this : &other;
                    Vector* big = this_size < other_size ? &other : this;
                    const usize min_size = small->size();
                    const usize max_size = big->size();

                    for(usize i = 0; i != min_size; ++i) {
                        std::swap(_data[i], other._data[i]);
                    }
                    for(usize i = min_size; i != max_size; ++i) {
                        ::new(&small->_data[i]) data_type(std::move(big->_data[i]));
                        big->_data[i].~data_type();
                    }
                    small->_data_end = small->_data + max_size;
                    big->_data_end = big->_data + min_size;
                    y_debug_assert(small->size() <= small->capacity());
                    y_debug_assert(big->size() <= big->capacity());
                } else if(is_sbo_active()) {
                    y_debug_assert(!other.is_sbo_active());
                    data_type* data = std::exchange(other._data, other.sbo_buffer());
                    data_type* data_end = std::exchange(other._data_end, other._data + size());
                    data_type* alloc_end = std::exchange(other._alloc_end, other._data + SBOCapacity::value);
                    move_range(other._data, _data, size());
                    _data = data;
                    _data_end = data_end;
                    _alloc_end = alloc_end;
                } else if(other.is_sbo_active()) {
                    y_debug_assert(!is_sbo_active());
                    other.swap(*this);
                } else {
                    return false;
                }
                return true;
            }
            return false;
        }

        inline bool is_sbo_active() {
            if constexpr(has_sbo) {
                return _data == this->sbo_buffer();
            }
            return false;
        }

        inline bool is_full() const {
            return _data_end == _alloc_end;
        }

        inline bool contains_it(const_iterator it) const {
            return it >= _data && it < _data_end;
        }

        inline void move_range(data_type* dst, data_type* src, usize n) {
            if constexpr(std::is_trivial_v<data_type>) {
                std::copy_n(src, n, dst);
            } else {
                for(usize i = 0; i != n; ++i) {
                    ::new(&dst[i]) data_type{std::move(src[i])};
                }
            }
        }

        inline void clear(data_type* beg, data_type* en) {
            if(!std::is_trivial_v<data_type>) {
                for(data_type* e = en; e != beg;) {
                    (--e)->~data_type();
                }
            }
        }

        inline void expand() {
            unsafe_set_capacity(ResizePolicy::ideal_capacity(size() + 1));
        }

        inline void shrink() {
            const usize current = size();
            const usize cap = capacity();
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
            const usize num_to_keep = new_cap < current_size ? new_cap : current_size;

            data_type* new_data = nullptr;
            if(new_cap) {
                new_data = (new_cap <= SBOCapacity::value)
                    ? this->sbo_buffer()
                    : Allocator::allocate(new_cap);
            }

            if(new_data != _data) {
                move_range(new_data, _data, num_to_keep);
                clear(_data, _data_end);

                if(_data && _data != this->sbo_buffer()) {
                    Allocator::deallocate(_data, capacity());
                }
            } else {
                clear(_data + num_to_keep, _data + current_size);
            }


            _data = new_data;
            _data_end = _data + num_to_keep;
            _alloc_end = _data + new_cap;
        }

        data_type* _data = nullptr;
        data_type* _data_end = nullptr;
        data_type* _alloc_end = nullptr;
};


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



template<typename Elem, usize Capacity = 16, typename Allocator = std::allocator<Elem>>
using SmallVector = Vector<Elem, Allocator, std::integral_constant<usize, Capacity>>;

}
}


#endif

