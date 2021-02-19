/*******************************
Copyright (c) 2016-2021 Gr�goire Angerand

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
#ifndef Y_REFLECT_TRAITS_H
#define Y_REFLECT_TRAITS_H

#include <y/core/Span.h>
#include <y/core/Range.h>

#include <y/utils/detect.h>
#include <y/utils/traits.h>

#include <memory>

namespace y {
namespace reflect {

namespace detail {
template<typename T>
using has_reflect_t = decltype(std::declval<T>()._y_reflect());
}

template<typename T>
static constexpr bool has_reflect_v = is_detected_v<detail::has_reflect_t, T>;



namespace detail {

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
}


template<typename T>
static constexpr bool is_range_v = detail::IsRange<remove_cvref_t<T>>::value;

template<typename T>
static constexpr bool is_tuple_v = detail::IsTuple<remove_cvref_t<T>>::value;

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

#endif // Y_REFLECT_TRAITS_H

