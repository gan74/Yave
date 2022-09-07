/*******************************
Copyright (c) 2016-2022 Gr�goire Angerand

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
#ifndef YAVE_SCRIPT_SOL_HELPERS_H
#define YAVE_SCRIPT_SOL_HELPERS_H

#include <yave/yave.h>

#include <y/core/String.h>
#include <y/core/Result.h>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>


namespace yave {
namespace script {

namespace detail {
template<typename T, typename M>
auto property(M T::* member) {
    if constexpr(std::is_same_v<M, core::String>) {
        return sol::property(
            [=](T* obj, std::string_view view) { (obj->*member) = view; },
            [=](T* obj) { return (obj->*member).view(); }
        );
    } else {
        return member;
    }
}

struct CollectionData {
    u64 id = 0;
};

CollectionData& get_collection_data(lua_State* l);
}


template<typename T>
struct Weak {
    T* ptr = nullptr;
    u64 collection_id = 0;
    mutable detail::CollectionData* collection_data = nullptr;

    Weak() = default;

    Weak(T* p) : ptr(p) {
    }
};

void clear_weak_refs(lua_State* l);

}
}



namespace sol {
template<typename T>
struct unique_usertype_traits<yave::script::Weak<T>> {
    using type = T;
    using actual_type = yave::script::Weak<T>;
    static const bool value = true;

    static bool is_null(lua_State*l, const actual_type& ptr) {
        if(!ptr.ptr) {
            return true;
        }

        if(!ptr.collection_data) {
            ptr.collection_data = &yave::script::detail::get_collection_data(l);
        }

        return ptr.collection_id < ptr.collection_data->id;
    }

    static type* get(const actual_type& ptr) {
        return ptr.ptr;
    }
};
}



inline int sol_lua_push(sol::types<y::core::String>, lua_State* l, const y::core::String& str) {
    return sol::stack::push(l, str.view());
}

inline y::core::String sol_lua_get(sol::types<y::core::String>, lua_State* l, int index, sol::stack::record& tracking) {
    return y::core::String(sol::stack::get<std::string_view>(l, lua_absindex(l, index)));
}

template <typename Handler>
inline bool sol_lua_check(sol::types<y::core::String>, lua_State* l, int index, Handler&& handler, sol::stack::record& tracking) {
    tracking.use(1);
    return sol::stack::check<std::string_view>(l, lua_absindex(l, index), handler);
}

#endif // YAVE_SCRIPT_SOL_HELPERS_H
