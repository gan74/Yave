/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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
#ifndef Y_UTILS_ITER_H
#define Y_UTILS_ITER_H

#include "traits.h"

#include <y/core/Range.h>

namespace y {

struct EndIterator {};

// this might cause ambiguities
template<typename T>
inline bool operator==(EndIterator, const T& other) {
    return other.at_end();
}

template<typename T>
inline bool operator==(const T& other, EndIterator) {
    return other.at_end();
}

template<typename T>
inline bool operator!=(EndIterator, const T& other) {
    return !other.at_end();
}

template<typename T>
inline bool operator!=(const T& other, EndIterator) {
    return !other.at_end();
}




template<typename It, typename Transform>
class TransformIterator : private Transform {
    using iterator_type = It;
    using iterator_traits = std::iterator_traits<iterator_type>;

    using raw_value_type = decltype(std::declval<Transform>()(*std::declval<It>()));

    public:
        using value_type = std::remove_reference_t<raw_value_type>;

        using difference_type = typename iterator_traits::difference_type;
        using iterator_category = typename iterator_traits::iterator_category;

        using reference = value_type&;
        using pointer = value_type*;

        inline TransformIterator() = default;
        inline TransformIterator(TransformIterator&&) = default;
        inline TransformIterator(const TransformIterator&) = default;

        inline TransformIterator& operator=(TransformIterator&& other) {
            _it = std::move(other._it);
            Transform::operator=(std::move(other));
            return *this;
        }

        inline TransformIterator& operator=(const TransformIterator& other) {
            _it = other._it;
            Transform::operator=(other);
            return *this;
        }

        inline TransformIterator(iterator_type it, const Transform& tr = Transform()) : Transform(tr), _it(it) {
        }

        //template<typename = std::enable_if_t<has_at_end_v<iterator_type>>>
        inline bool at_end() const {
            static_assert(has_at_end_v<iterator_type>);
            return _it.at_end();
        }

        inline TransformIterator& operator++() {
            ++_it;
            return *this;
        }

        inline TransformIterator operator++(int) {
            const iterator_type it = _it;
            ++_it;
            return TransformIterator(it, *this);
        }

        inline TransformIterator& operator--() {
            --_it;
            return *this;
        }

        inline TransformIterator operator--(int) {
            const iterator_type it = _it;
            --_it;
            return TransformIterator(it, *this);
        }

        inline bool operator==(const TransformIterator& other) const {
            return _it == other._it;
        }

        inline bool operator!=(const TransformIterator& other) const {
            return _it != other._it;
        }

        inline decltype(auto) operator*() const {
            return Transform::operator()(*_it);
        }

        inline auto* operator->() const {
            static_assert(std::is_reference_v<raw_value_type>);
            return &Transform::operator()(*_it);
        }


        inline TransformIterator operator+(usize i) const {
            return TransformIterator(_it + i, *this);
        }

        inline TransformIterator operator-(usize i) const {
            return TransformIterator(_it - i, *this);
        }

        inline TransformIterator& operator+=(usize i) const {
            _it += i;
            return *this;
        }

        inline TransformIterator& operator-=(usize i) const {
            _it -= i;
            return *this;
        }

        inline const iterator_type& inner() const {
            return _it;
        }

    private:
        iterator_type _it;
};

template<typename It, typename Filter, typename End = It>
class FilterIterator : private Filter {
    using iterator_type = It;
    using end_iterator_type = End;
    using iterator_traits = std::iterator_traits<iterator_type>;

    public:
        using value_type = std::remove_reference_t<decltype(*std::declval<It>())>;

        using difference_type = typename iterator_traits::difference_type;
        using iterator_category = std::forward_iterator_tag;

        using reference = value_type&;
        using pointer = value_type*;

        inline FilterIterator() = default;
        inline FilterIterator(FilterIterator&&) = default;
        inline FilterIterator(const FilterIterator&) = default;

        inline FilterIterator& operator=(FilterIterator&& other) {
            _it = std::move(other._it);
            _end = std::move(other._end);
            Filter::operator=(std::move(other));
            return *this;
        }

        inline FilterIterator& operator=(const FilterIterator& other) {
            _it = other._it;
            _end = other._end;
            Filter::operator=(other);
            return *this;
        }

        inline FilterIterator(iterator_type it, end_iterator_type end, const Filter& ft = Filter()) : Filter(ft), _it(it), _end(end) {
            find_next_valid();
        }

        inline void advance() {
            y_debug_assert(!at_end());
            ++_it;
            find_next_valid();
        }

        inline bool at_end() const {
            return _it == _end;
        }

        inline FilterIterator& operator++() {
            advance();
            return *this;
        }

        inline FilterIterator operator++(int) {
            const iterator_type it = _it;
            advance();
            return FilterIterator(it, _end, *this);
        }

        inline bool operator==(const FilterIterator& other) const {
            return _it == other._it;
        }

        inline bool operator!=(const FilterIterator& other) const {
            return _it != other._it;
        }

        inline decltype(auto) operator*() const {
            y_debug_assert(!at_end());
            return *_it;
        }

        inline auto* operator->() const {
            y_debug_assert(!at_end());
            return *_it;
        }

        inline const iterator_type& inner() const {
            return _it;
        }

    private:
        inline void find_next_valid() {
            while(!at_end()) {
                if(Filter::operator()(*_it)) {
                    break;
                }
                ++_it;
            }
        }

        iterator_type _it;
        end_iterator_type _end;
};




template<typename It, typename T>
inline bool operator==(const It& lhs, const TransformIterator<It, T>& rhs) {
    return lhs == rhs.inner();
}

template<typename It, typename T>
inline bool operator==(const TransformIterator<It, T>& lhs, const It& rhs) {
    return lhs.inner() == rhs;
}

template<typename It, typename T>
inline bool operator!=(const It& lhs, const TransformIterator<It, T>& rhs) {
    return lhs != rhs.inner();
}

template<typename It, typename T>
inline bool operator!=(const TransformIterator<It, T>& lhs, const It& rhs) {
    return lhs.inner() != rhs;
}

template<typename It, typename T>
inline auto operator-(const It& lhs, const TransformIterator<It, T>& rhs) {
    return lhs - rhs.inner();
}

template<typename It, typename T>
inline auto operator-(const TransformIterator<It, T>& lhs, const It& rhs) {
    return lhs.inner() - rhs;
}



template<typename It, typename T>
inline bool operator==(const It& lhs, const FilterIterator<It, T>& rhs) {
    return lhs == rhs.inner();
}

template<typename It, typename T>
inline bool operator==(const FilterIterator<It, T>& lhs, const It& rhs) {
    return lhs.inner() == rhs;
}

template<typename It, typename T>
inline bool operator!=(const It& lhs, const FilterIterator<It, T>& rhs) {
    return lhs != rhs.inner();
}

template<typename It, typename T>
inline bool operator!=(const FilterIterator<It, T>& lhs, const It& rhs) {
    return lhs.inner() != rhs;
}

}

#endif // Y_UTILS_ITER_H

