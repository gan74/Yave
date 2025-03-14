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

#include <y/core/Vector.h>

#include <y/utils/name.h>
#include <y/utils/format.h>

#include <external/LuaJIT/src/lua.hpp>

#include <tuple>
#include <type_traits>


#include <y/utils/log.h>

namespace yave {
namespace lua {

template<typename T>
struct LuaUserData {
        T value;
};

template<typename T>
y_force_inline static const char* metatable_name() {
    static_assert(!std::is_reference_v<T>);
    static_assert(!std::is_const_v<T>);
    return ct_type_name<T>().data();
}

template<typename T>
y_force_inline auto& get_user_value(lua_State* L, int index) {
    using inner = std::remove_cvref_t<T>;
    LuaUserData<inner>* userdata = static_cast<LuaUserData<inner>*>(luaL_checkudata(L, index, metatable_name<inner>()));
    return userdata->value;
}

template<typename T>
y_force_inline static void push_value(lua_State* L, T val) {
    using non_ref = std::remove_reference_t<T>;

    if constexpr(std::is_same_v<non_ref, bool>) {
        lua_pushboolean(L, int(val));
    } else if constexpr(std::is_integral_v<non_ref>) {
        lua_pushinteger(L, int(val));
    } else if constexpr(std::is_floating_point_v<non_ref>) {
        lua_pushnumber(L, lua_Number(val));
    } else if constexpr(std::is_convertible_v<non_ref, std::string_view>) {
        const std::string_view v(val);
        lua_pushlstring(L, v.data(), v.size());
    } else {
        static_assert(false, "Unsupported type");
    }
}

template<typename T>
y_force_inline T get_value(lua_State* L, int index) {
    static_assert(std::is_trivially_destructible_v<T>);

    using non_ref = std::remove_reference_t<T>;
    constexpr bool is_ref = std::is_reference_v<T>;

    if constexpr(std::is_same_v<non_ref, bool>) {
        static_assert(!is_ref);
        return !!luaL_checkinteger(L, index);
    } else if constexpr(std::is_integral_v<non_ref>) {
        static_assert(!is_ref);
        return T(luaL_checkinteger(L, index));
    } else if constexpr(std::is_floating_point_v<non_ref>) {
        static_assert(!is_ref);
        return T(luaL_checknumber(L, index));
    } else if constexpr(std::is_convertible_v<std::string_view, non_ref> && !is_ref) {
        usize size = 0;
        const char* str = luaL_checklstring(L, index, &size);
        return T(std::string_view(str, size));
    } else {
        return get_user_value<T>(L, index);
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


template<typename T>
void bind_type(lua_State* L, const char* name) {
    core::SmallVector<luaL_Reg, 13> methods;

    {
        if constexpr(!std::is_trivially_destructible_v<T>) {
            methods.emplace_back(
                "__gc", [](lua_State* L) {
                    get_user_value<T>(L, 1).~T();
                    return 0;
                }
            );
        }

        methods.emplace_back(
            "__tostring", [](lua_State* L) {
                lua_pushfstring(L, fmt_c_str("userdata({})", metatable_name<T>()));
                return 1;
            }
        );

        methods.emplace_back(nullptr, nullptr);
    }

    luaL_newmetatable(L, metatable_name<T>());
    luaL_setfuncs(L, methods.data(), 0);

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    auto new_adaptor = [](lua_State* L) {
        LuaUserData<T>* userdata = static_cast<LuaUserData<T>*>(lua_newuserdata(L, sizeof(LuaUserData<T>)));
        new (&userdata->value) T();
        luaL_getmetatable(L, metatable_name<T>());
        lua_setmetatable(L, -2);
        return 1;
    };

    lua_newtable(L);
    lua_pushcfunction(L, new_adaptor);

    lua_setfield(L, -2, "new");
    lua_setglobal(L, name);
}


}
}


#endif // YAVE_SCRIPT_LUAHELPERS_H

