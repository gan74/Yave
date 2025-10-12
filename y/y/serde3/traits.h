/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
concept has_no_serde3_defined = requires(T t) {
    t._y_serde3_no_serde;
};

template<typename T>
static inline consteval bool has_no_serde3_impl() {
    if constexpr(has_no_serde3_defined<T>) {
        return T::_y_serde3_no_serde;
    }
    return false;
}
}

template<typename T>
concept has_serde3 = has_reflect<T>;

template<typename T>
concept has_no_serde3 = detail::has_no_serde3_impl<T>();


template<typename T, typename... Args>
concept has_serde3_post_deser = requires(T t, Args... args) {
    t.post_deserialize(args...);
};

template<typename T, typename... Args>
concept has_serde3_post_deser_poly = requires(T t, Args... args) {
    t.post_deserialize_poly(args...);
};

template<typename T>
concept has_serde3_poly = requires(T t) {
    t._y_serde3_poly_base;
};

template<typename T>
concept has_serde3_ptr_poly = requires(T t) {
    t->_y_serde3_poly_base;
};



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

template<typename T, typename value_type = std::remove_cvref_t<typename T::value_type>>
constexpr bool use_collection_fast_path =
        (has_resize<T> || has_emplace_back<T>) &&
        std::is_pointer_v<decltype(std::declval<T>().begin())> &&
        std::is_trivially_copyable_v<value_type> &&
        !has_serde3<value_type> &&
        !std::is_pointer_v<value_type>;

template<typename T>
concept is_pod_base = std::is_trivially_copyable_v<std::remove_cvref_t<T>> && std::is_trivially_copy_constructible_v<std::remove_cvref_t<T>>;

template<typename T>
consteval bool is_pod_iterable() {
    if constexpr(is_iterable<T>) {
        using value_type = decltype(*std::declval<T>().begin());
        return is_pod_base<value_type>;
    }
    return true;
}

}

// Warning: some types like Range and Span are can be POD (Span is handled separatly tho)
template<typename T>
concept is_pod = detail::is_pod_base<T> && detail::is_pod_iterable<std::remove_cvref_t<T>>();

template<typename T>
concept is_property = detail::IsProperty<std::remove_cvref_t<T>>::value;

}
}

#endif // Y_SERDE3_TRAITS_H

