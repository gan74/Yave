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
#ifndef YAVE_SCRIPT_LUAHELPERS_H
#define YAVE_SCRIPT_LUAHELPERS_H


#include <yave/yave.h>

#include <external/LuaJIT/src/lua.hpp>

#include <tuple>

namespace yave {
namespace lua {

template<typename T>
y_force_inline static void push_value(lua_State* L, T val) {
    if constexpr(std::is_same_v<T, bool>) {
        lua_pushboolean(L, int(val));
    } else if constexpr(std::is_integral_v<T>) {
        lua_pushinteger(L, int(val));
    } else if constexpr(std::is_floating_point_v<T>) {
        lua_pushnumber(L, lua_Number(val));
    } else if constexpr(std::is_convertible_v<T, std::string_view>) {
        const std::string_view v(val);
        lua_pushlstring(L, v.data(), v.size());
    } else {
        static_assert(false, "Unsupported type");
    }
}

template<typename T>
y_force_inline T get_value(lua_State* L, int index) {
    static_assert(std::is_trivially_destructible_v<T>);
    if constexpr(std::is_same_v<T, bool>) {
        return !!luaL_checkinteger(L, index);
    } else if constexpr(std::is_integral_v<T>) {
        return T(luaL_checkinteger(L, index));
    } else if constexpr(std::is_floating_point_v<T>) {
        return T(luaL_checknumber(L, index));
    } else if constexpr(std::is_convertible_v<std::string_view, T>) {
        usize size = 0;
        const char* str = luaL_checklstring(L, index, &size);
        return T(std::string_view(str, size));
    } else {
        static_assert(false, "Unsupported type");
    }
}

template<typename T, usize... Is>
y_force_inline T get_values(lua_State* L, std::index_sequence<Is...>) {
    static_assert(std::is_trivially_destructible_v<T>);
    static_assert(std::tuple_size_v<T> == sizeof...(Is));
    return T(
        get_value<std::tuple_element_t<Is, T>>(L, int(Is + 1))...
    );
}

template<auto F>
void bind_func(lua_State* L, const char* name) {
    auto adaptor = [](lua_State* L) {
        using func = function_traits<decltype(F)>;
        if constexpr(std::is_void_v<typename func::return_type>) {
            std::apply(F, get_values<typename func::argument_pack>(L, std::make_index_sequence<func::arg_count>()));
            return 0;
        } else {
            push_value(L, std::apply(F, get_values<typename func::argument_pack>(L, std::make_index_sequence<func::arg_count>())));
            return 1;
        }
    };
    
    lua_pushcfunction(L, adaptor);
    lua_setglobal(L, name);
}

}
}


#endif // YAVE_SCRIPT_LUAHELPERS_H

