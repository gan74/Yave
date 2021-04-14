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
#ifndef Y_SERDE3_TRAITS_H
#define Y_SERDE3_TRAITS_H

#include "serde.h"

#include <y/reflect/traits.h>

#include <memory>

namespace y {
namespace serde3 {

namespace detail {
template<typename T>
using has_no_serde3_t = decltype(std::declval<T>()._y_serde3_no_serde);

template<typename T>
static inline constexpr bool has_no_serde3_impl() {
    if constexpr(is_detected_v<has_no_serde3_t, T>) {
        return T::_y_serde3_no_serde;
    }
    return false;
}


template<typename T, typename... Args>
using has_serde3_post_deser_t = decltype(std::declval<T>().post_deserialize(std::declval<Args>()...));
template<typename T, typename... Args>
using has_serde3_post_deser_poly_t = decltype(std::declval<T>().post_deserialize_poly(std::declval<Args>()...));


template<typename T>
using has_serde3_poly_t = decltype(std::declval<T>()._y_serde3_poly_base);
template<typename T>
using has_serde3_ptr_poly_t = decltype(std::declval<T>()->_y_serde3_poly_base);

}

template<typename T>
static constexpr bool has_serde3_v = has_reflect_v<T>;

template<typename T>
static constexpr bool has_no_serde3_v = detail::has_no_serde3_impl<T>();


template<typename T, typename... Args>
static constexpr bool has_serde3_post_deser_v = is_detected_v<detail::has_serde3_post_deser_t, T, Args...>;

template<typename T, typename... Args>
static constexpr bool has_serde3_post_deser_poly_v = is_detected_v<detail::has_serde3_post_deser_poly_t, T, Args...>;
template<typename T>
static constexpr bool has_serde3_poly_v = is_detected_v<detail::has_serde3_poly_t, T>;

template<typename T>
static constexpr bool has_serde3_ptr_poly_v = is_detected_v<detail::has_serde3_ptr_poly_t, T>;


template<typename T>
constexpr auto members(T&& t) {
    if constexpr(has_serde3_v<T>) {
        return t._y_reflect();
    } else {
        return std::tuple<>{};
    }
}

template<typename T>
constexpr usize member_count() {
    return std::tuple_size_v<decltype(members(std::declval<T&>()))>;
}





namespace detail {

template<typename T, typename G, typename S>
class Property;

template<typename T>
struct IsProperty {
    static constexpr bool value = false;
};

template<typename T, typename G, typename S>
struct IsProperty<detail::Property<T, G, S>> {
    static constexpr bool value = true;
};

template<typename T, typename value_type = remove_cvref_t<typename T::value_type>>
constexpr bool use_collection_fast_path =
        (has_resize_v<T> || has_emplace_back_v<T>) &&
        std::is_pointer_v<decltype(std::declval<T>().begin())> &&
        std::is_trivially_copyable_v<value_type> &&
        !has_serde3_v<value_type> &&
        !std::is_pointer_v<value_type>;

template<typename T>
static constexpr bool is_pod_base_v = std::is_trivially_copyable_v<remove_cvref_t<T>> && std::is_trivially_copy_constructible_v<remove_cvref_t<T>>;

template<typename T>
constexpr bool is_pod_iterable() {
    if constexpr(is_iterable_v<T>) {
        using value_type = decltype(*std::declval<T>().begin());
        return is_pod_base_v<value_type>;
    }
    return true;
}

}

// Warning: some types like Range and Span are can be POD (Span is handled separatly tho)
template<typename T>
static constexpr bool is_pod_v = detail::is_pod_base_v<T> && detail::is_pod_iterable<remove_cvref_t<T>>();

template<typename T>
static constexpr bool is_property_v = detail::IsProperty<remove_cvref_t<T>>::value;

}
}

#endif // Y_SERDE3_TRAITS_H

