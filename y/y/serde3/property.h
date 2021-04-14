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
#ifndef Y_SERDE3_PROPERTY_H
#define Y_SERDE3_PROPERTY_H

#include <y/utils/traits.h>

namespace y {
namespace serde3 {

namespace detail {
template<typename T, typename G, typename S>
class Property {
    public:
        static constexpr bool return_ref = std::is_reference_v<G>;

        using value_type = remove_cvref_t<G>;

        ~Property() {
        }

        G get() const {
            y_debug_assert(_const_self && _get);
            return (_const_self->*_get)();
        }

        template<typename A>
        void set(A&& t) {
            y_debug_assert(_self && _set);
            return (_self->*_set)(y_fwd(t));
        }


        const T* _const_self = nullptr;
        G (T::*_get)() const = nullptr;

        T* _self = nullptr;
        void (T::*_set)(S) = nullptr;
};
}

template<typename T, typename G, typename S>
auto property(T* self, G (T::*get)() const, void (T::*set)(S)) {
    return detail::Property<T, G, S>{self, get, self, set};
}

template<typename T, typename G, typename S>
auto property(const T* self, G (T::*get)() const, void (T::*)(S)) {
    return detail::Property<T, G, S>{self, get, nullptr, nullptr};
}

}
}

#endif // Y_SERDE3_PROPERTY_H

