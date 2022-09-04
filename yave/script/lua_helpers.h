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

namespace detail {
struct no_reload_tag {};

template<typename T>
struct UData {
    // T* ptr = nullptr;
    T obj = {};
};

static inline std::string_view check_string_view(lua_State* l, int idx) {
    usize len = 0;
    const char* name_ptr = luaL_checklstring(l, idx, &len);
    return std::string_view(name_ptr, len);
}

template<typename T>
static inline void get_arg_value(lua_State* l, T& value, int idx = -1) {
    if constexpr(std::is_same_v<T, core::String>) {
        value = check_string_view(l, idx);
    } else if constexpr(std::is_floating_point_v<T>) {
        value = T(luaL_checknumber(l, idx));
    } else {
        static_assert(std::is_integral_v<T>, "Unable to get value");
        value = T(luaL_checkinteger(l, idx));
    }
}

template<int I, typename Tpl>
static inline void collect_args_internal(lua_State* l, Tpl& args) {
    if constexpr(I < std::tuple_size_v<Tpl>) {
        get_arg_value(l, std::get<I>(args), I + 1);
        collect_args_internal<I + 1>(l, args);
    }
}

template<typename Tpl>
Tpl collect_args(lua_State* l) {
    Tpl args = {};
    collect_args_internal<0>(l, args);
    return args;
}
}



template<typename T>
void push_value(lua_State* l, const T& value) {
    if constexpr(std::is_trivially_constructible_v<const char*, T>) {
        lua_pushstring(l, value);
    } else if constexpr(std::is_convertible_v<T, lua_CFunction>) {
        lua_pushcfunction(l, value);
    } else if constexpr(std::is_same_v<T, core::String>) {
        lua_pushstring(l, value.data());
    } else if constexpr(std::is_floating_point_v<T>) {
        lua_pushnumber(l, lua_Number(value));
    } else {
        static_assert(std::is_integral_v<T>, "Unable to push value");
        lua_pushinteger(l, lua_Integer(value));
    }
}

template<typename T, typename F = detail::no_reload_tag>
void create_type_metatable(lua_State* l, F&& reload = F{}) {
    static_assert(reflect::has_reflect_v<T>);
    static_assert(std::is_same_v<F, detail::no_reload_tag>);

    using UData = detail::UData<T>;

    auto get = [](lua_State* l) -> int {
        const std::string_view name = detail::check_string_view(l, -1);
        const T* ptr = &static_cast<const UData*>(lua_touserdata(l, -2))->obj;

        if(!ptr) {
            return 0;
        }

        int ret = 0;
        reflect::explore_members<T>([&](std::string_view member_name, auto member) {
            if(member_name == name) {
                y_debug_assert(!ret);
                ret = 1;
                push_value(l, ptr->*member);
            }
        });

        return ret;
    };

    auto set = [](lua_State* l) -> int {
        const std::string_view name = detail::check_string_view(l, -2);
        T* ptr = &static_cast<UData*>(lua_touserdata(l, -3))->obj;

        if(!ptr) {
            return 0;
        }

        reflect::explore_members<T>([&](std::string_view member_name, auto member) {
            if(member_name == name) {
                detail::get_arg_value(l, ptr->*member);
            }
        });

        return 0;
    };

    auto create = [](lua_State* l) -> int {
        void* mem = lua_newuserdata(l, sizeof(UData));
        UData* udata = new(mem) UData();

        lua_pushlightuserdata(l, create_type_metatable<T>);
        lua_rawget(l, LUA_REGISTRYINDEX);
        y_debug_assert(lua_istable(l, -1));

        lua_setmetatable(l, -2);

        return 1;
    };

    auto destroy = [](lua_State* l) -> int {
        UData* udata = static_cast<UData*>(lua_touserdata(l, -1));
        y_debug_assert(udata);

        udata->~UData();

        return 0;
    };

    lua_createtable(l, 0, 6);
    lua_pushcfunction(l, get);
    lua_setfield(l, -2, "__index");

    lua_pushcfunction(l, set);
    lua_setfield(l, -2, "__newindex");

    lua_pushcfunction(l, destroy);
    lua_setfield(l, -2, "__gc");

    lua_pushcfunction(l, create);
    lua_setfield(l, -2, "__create");

    lua_pushstring(l, T::_y_reflect_type_name);
    lua_setfield(l, -2, "__typename");

    lua_pushvalue(l, -2);
    lua_setfield(l, -2, "__metatable");

    lua_pushlightuserdata(l, create_type_metatable<T>);
    lua_pushvalue(l, -2);
    lua_rawset(l, LUA_REGISTRYINDEX);

    {
        lua_createtable(l, 0, 1);
        lua_pushcfunction(l, create);
        lua_setfield(l, -2, "new");

        lua_setglobal(l, T::_y_reflect_type_name);
    }
}

template<auto F>
int bind_function(lua_State* l) {
    using traits = function_traits<decltype(F)>;
    auto args = detail::collect_args<traits::argument_pack>(l);
    if constexpr(std::is_void_v<typename traits::return_type>) {
        std::apply(F, std::move(args));
        return 0;
    } else {
        push_value(l, std::apply(F, std::move(args)));
        return 1;
    }
}

}
}
}

#endif // YAVE_SCRIPT_LUA_HELPERS_H
