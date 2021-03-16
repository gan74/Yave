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
#ifndef Y_REFLECT_DYN_H
#define Y_REFLECT_DYN_H

#include "reflect.h"

#include <typeindex>
#include <array>

namespace y {
namespace reflect {

template<bool Const = false>
struct DynObject {
    static constexpr bool is_const = Const;

    template<typename T>
    using to_ptr_t = std::conditional_t<Const, const T*, T*>;
    using ptr_t = to_ptr_t<void>;

    ptr_t obj = nullptr;
    std::type_index type = typeid(void);
    std::string_view name;

    template<typename T>
    bool is() const {
        return type == typeid(remove_cvref_t<T>);
    }

    template<typename T>
    to_ptr_t<T> to() const {
        return is<T>() ? static_cast<to_ptr_t<T>>(obj) : nullptr;
    }

    DynObject<true> to_const() const {
        return DynObject<true> {
            obj, type, name
        };
    }
};

namespace detail {
template<typename T>
inline auto create_dyn_member(std::string_view name, T& t) {
    return DynObject<false>{&t, typeid(remove_cvref_t<T>), name};
}


template<typename T>
inline auto create_dyn_member(std::string_view name, const T& t) {
    return DynObject<true>{&t, typeid(remove_cvref_t<T>), name};
}
}

template<typename T, typename F>
void explore_dyn(T&& obj, F&& func) {
    static_assert(has_reflect_v<T>);
    explore_members(y_fwd(obj), [&](std::string_view name, auto&& m) {
        func(detail::create_dyn_member(name, y_fwd(m)));
    });
}

}
}

#endif // Y_REFLECT_DYN_H

