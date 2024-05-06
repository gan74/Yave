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
#ifndef YAVE_ECS_TRAITS_H
#define YAVE_ECS_TRAITS_H

#include "ecs.h"

#include <y/utils/traits.h>

namespace yave {
namespace ecs {


template<typename T>
struct Mutate {};

template<typename T>
struct Changed {};



namespace traits {
template<typename T>
struct component_type {
    using raw_type = std::remove_cvref_t<T>;
    using type = const raw_type;
    using discard_query_quals = T;

    static constexpr bool changed = false;
};

template<typename T>
struct component_type<Mutate<T>> {
    using raw_type = typename component_type<T>::raw_type;
    using type = std::remove_const_t<typename component_type<T>::type>;
    using discard_query_quals = Mutate<T>;

    static constexpr bool changed = component_type<T>::changed;
};

template<typename T>
struct component_type<Changed<T>> {
    using raw_type = typename component_type<T>::raw_type;
    using type = typename component_type<T>::type;
    using discard_query_quals = T;

    static constexpr bool changed = true;
};






template<typename T>
using component_raw_type_t = typename component_type<T>::raw_type;

template<typename T>
using component_type_t = typename component_type<T>::type;

template<typename T>
using discard_query_qualifiers_t = typename component_type<T>::discard_query_quals;

template<typename T>
static constexpr bool is_component_changed_v = component_type<T>::changed;

template<typename T>
static constexpr bool is_component_const_v = std::is_const_v<typename component_type<T>::type>;

template<typename T>
static constexpr bool is_component_mutable_v = !is_component_const_v<T>;

}

}
}

#endif // YAVE_ECS_TRAITS_H
