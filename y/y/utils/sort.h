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
#ifndef Y_UTILS_SORT_H
#define Y_UTILS_SORT_H

#include "types.h"

#include <array>
#include <algorithm>


namespace y {

// waiting for C++20
template<typename It, typename C = std::less<>>
inline constexpr bool is_sorted(It b, It e, C comp = C()) {
    if(b != e) {
        const auto next = b;
        while(++next != e) {
            if(comp(*next, *b)) {
                return false;
            }
            b = next;
        }
    }
    return true;
}

namespace detail {
template<typename T, usize N, typename C>
static constexpr void ct_sort(std::array<T, N>& arr, usize left, usize right, C comp) {
    auto ct_swap = [](T& lhs, T& rhs) {
        T tmp = std::move(lhs);
        lhs = std::move(rhs);
        rhs = std::move(tmp);
    };

    if(left < right) {
        usize m = left;

        for(usize i = left + 1; i != right; ++i) {
            if(comp(arr[i], arr[left])) {
                ct_swap(arr[++m], arr[i]);
            }
        }

        ct_swap(arr[left], arr[m]);

        ct_sort(arr, left, m);
        ct_sort(arr, m + 1, right);
    }
}
}

template <typename T, usize N, typename C = std::less<>>
constexpr std::array<T, N> ct_sort(std::array<T, N> arr, C comp = C()) {
    const std::array<T, N> sorted = arr;
    detail::ct_sort(sorted, 0, N, comp);
    return sorted;
}


// https://codereview.stackexchange.com/questions/131194/selection-sorting-a-type-list-compile-time
namespace detail {

template<usize I, usize J, typename Tuple>
struct tuple_element_swap {
    private:
        template<typename IndexSequence>
        struct tuple_element_swap_impl;

        template<usize... Indices>
        struct tuple_element_swap_impl<std::index_sequence<Indices...>> {
            using type = std::tuple<std::tuple_element_t<Indices != I && Indices != J ? Indices : Indices == I ? J : I, Tuple>...>;
        };

    public:
        using type = typename tuple_element_swap_impl<std::make_index_sequence<std::tuple_size<Tuple>::value>>::type;
};

template<typename Tuple, template<typename, typename> typename Comparator>
struct tuple_sort {
    private:
        template<usize I, usize J, usize TupleSize, typename LoopTuple>
        struct tuple_sort_impl {
            using tuple_type = std::conditional_t<Comparator<
                    const std::tuple_element_t<I, LoopTuple>,
                    const std::tuple_element_t<J, LoopTuple>
                >::value,
                typename detail::tuple_element_swap<I, J, LoopTuple>::type,
                LoopTuple>;

            using type = typename tuple_sort_impl<I, J + 1, TupleSize, tuple_type>::type;
        };

        template<usize I, usize TupleSize, typename LoopTuple>
        struct tuple_sort_impl<I, TupleSize, TupleSize, LoopTuple> {
            using type = typename tuple_sort_impl<I + 1, I + 2, TupleSize, LoopTuple>::type;
        };

        template<usize J, usize TupleSize, typename LoopTuple>
        struct tuple_sort_impl<TupleSize, J, TupleSize, LoopTuple> {
            using type = LoopTuple;
        };

    public:
        using type = typename tuple_sort_impl<0, 1, std::tuple_size<Tuple>::value, Tuple>::type;
};

}

template<typename T, template<typename, typename> typename Cmp>
using sorted_tuple_t = typename detail::tuple_sort<T, Cmp>::type;

}

#endif // Y_UTILS_SORT_H

