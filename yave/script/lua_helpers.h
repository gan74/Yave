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
#ifndef YAVE_SCRIPT_LUA_HELPERS_H
#define YAVE_SCRIPT_LUA_HELPERS_H

#include <yave/yave.h>

#include <y/core/String.h>

#include <y/reflect/reflect.h>

#include <external/LuaJIT/src/lua.hpp>

namespace yave {
namespace script {
namespace lua {

template<typename T>
static inline void push_value(lua_State* l, const T& value) {
    if constexpr(std::is_same_v<T, const char*>) {
        lua_pushstring(l, value);
    } else if constexpr(std::is_same_v<T, core::String>) {
        lua_pushstring(l, value.data());
    } else if constexpr(std::is_floating_point_v<T>) {
        lua_pushnumber(l, lua_Number(value));
    } else if constexpr(std::is_integral_v<T>) {
        lua_pushinteger(l, lua_Integer(value));
    } else {
        y_fatal("Unable to push value");
    }
}

template<typename T>
static inline void pop_value(lua_State* l, T& value) {
    if constexpr(std::is_same_v<T, core::String>) {
        value = lua_tostring(l, -1);
    } else if constexpr(std::is_floating_point_v<T>) {
        value = T(lua_tonumber(l, -1));
    } else if constexpr(std::is_integral_v<T>) {
        value = T(lua_tointeger(l, -1));
    } else {
        y_fatal("Unable to pop value");
    }
}

template<typename T>
int get_member(lua_State* l) {
    const std::string_view name = lua_tostring(l, -1);
    const T* ptr = static_cast<const T*>(lua_touserdata(l, -2));

    int ret = 0;
    reflect::explore_members<T>([&](std::string_view member_name, auto member) {
        if(member_name == name) {
            y_debug_assert(!ret);
            ret = 1;
            push_value(l, ptr->*member);
        }
    });

    return ret;
}

template<typename T>
int set_member(lua_State* l) {
    const std::string_view name = lua_tostring(l, -2);
    T* ptr = static_cast<T*>(lua_touserdata(l, -3));

    reflect::explore_members<T>([&](std::string_view member_name, auto member) {
        if(member_name == name) {
            pop_value(l, ptr->*member);
        }
    });

    return 0;
}

template<typename T>
void* registry_id() {
    return reinterpret_cast<void*>(&registry_id<T>);
}

template<typename T>
static inline void create_type_metatable(lua_State* l) {
    static_assert(reflect::has_reflect_v<T>);

    lua_createtable(l, 0, 2);
    lua_pushcfunction(l, get_member<T>);
    lua_setfield(l, -2, "__index");

    lua_pushcfunction(l, set_member<T>);
    lua_setfield(l, -2, "__newindex");

    lua_pushstring(l, T::_y_reflect_type_name);
    lua_setfield(l, -2, "__typename");

    lua_pushvalue(l, -2);
    lua_setfield(l, -2, "__metatable");

    lua_pushlightuserdata(l, registry_id<T>());
    lua_pushvalue(l, -2);
    lua_rawset(l, LUA_REGISTRYINDEX);
}

template<typename T>
int create_object(lua_State* l) {
    Y_TODO(fix alloc)
    void* ptr = static_cast<void*>(new T());
    lua_pushlightuserdata(l, ptr);

    lua_pushlightuserdata(l, registry_id<T>());
    lua_rawget(l, LUA_REGISTRYINDEX);
    y_debug_assert(lua_istable(l, -1));

    lua_setmetatable(l, -2);
    return 1;
}

}
}
}

#endif // YAVE_SCRIPT_LUA_HELPERS_H
