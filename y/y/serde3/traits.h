/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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

#include <y/core/Span.h>
#include <y/core/Range.h>

#include <y/utils/traits.h>

#include <memory>

namespace y {
namespace serde3 {
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

template<typename T>
struct IsRange {
    static constexpr bool value = false;
};

template<typename T>
struct IsRange<core::MutableSpan<T>> {
    static constexpr bool value = true;
};

template<typename I, typename E>
struct IsRange<core::Range<I, E>> {
    static constexpr bool value = true;
};


template<typename T>
struct IsTuple {
    static constexpr bool value = false;
};

template<typename... Args>
struct IsTuple<std::tuple<Args...>> {
    static constexpr bool value = true;
};

template<typename A, typename B>
struct IsTuple<std::pair<A, B>> {
    static constexpr bool value = true;
};


template<typename T>
struct StdPtr {
    static constexpr bool is_std_ptr = false;
};

template<typename T>
struct StdPtr<std::unique_ptr<T>> {
    static constexpr bool is_std_ptr = true;
    static auto make() {
        return std::make_unique<T>();
    }
};

template<typename T>
struct StdPtr<std::shared_ptr<T>> {
    static constexpr bool is_std_ptr = true;
    static auto make() {
        return std::make_shared<T>();
    }
};


template<typename T>
struct IsArray {
    static constexpr bool value = false;
};

template<typename T, usize N>
struct IsArray<std::array<T, N>> {
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

template<typename T>
static constexpr bool is_property_v = detail::IsProperty<remove_cvref_t<T>>::value;

template<typename T>
static constexpr bool is_range_v = detail::IsRange<remove_cvref_t<T>>::value;

template<typename T>
static constexpr bool is_tuple_v = detail::IsTuple<remove_cvref_t<T>>::value;

// Warning some types like Range and Span are can be POD (Span is handled separatly tho)
template<typename T>
static constexpr bool is_pod_v = detail::is_pod_base_v<T> && detail::is_pod_iterable<remove_cvref_t<T>>();

template<typename T>
static constexpr bool is_std_ptr_v = detail::StdPtr<remove_cvref_t<T>>::is_std_ptr;

template<typename T>
static constexpr bool is_array_v = detail::IsArray<remove_cvref_t<T>>::value;

template<typename T>
auto make_std_ptr() {
    static_assert(is_std_ptr_v<T>);
    return detail::StdPtr<T>::make();
}

}
}

#endif // Y_SERDE3_TRAITS_H

