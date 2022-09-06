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
#include <y/core/Vector.h>

#include <y/reflect/reflect.h>

#include <external/LuaJIT/src/lua.hpp>


namespace yave {
namespace script {
namespace lua {

template<typename T>
static constexpr bool can_push_value_v =
        std::is_same_v<T, bool> ||
        std::is_trivially_constructible_v<const char*, T> ||
        std::is_convertible_v<T, lua_CFunction> ||
        std::is_same_v<T, core::String> ||
        std::is_floating_point_v<T> ||
        std::is_integral_v<T>;

template<typename T>
static constexpr bool can_get_value_v =
        std::is_same_v<T, core::String> ||
        std::is_floating_point_v<T> ||
        std::is_integral_v<T>;

template<typename T>
static inline void push_value(lua_State* l, const T& value) {
    static_assert(can_push_value_v<T>, "Unable to push value");

    if constexpr(std::is_same_v<T, bool>) {
        lua_pushboolean(l, value ? 1 : 0);
    } else if constexpr(std::is_trivially_constructible_v<const char*, T>) {
        lua_pushstring(l, value);
    } else if constexpr(std::is_convertible_v<T, lua_CFunction>) {
        lua_pushcfunction(l, value);
    } else if constexpr(std::is_same_v<T, core::String>) {
        lua_pushstring(l, value.data());
    } else if constexpr(std::is_floating_point_v<T>) {
        lua_pushnumber(l, lua_Number(value));
    } else {
        static_assert(std::is_integral_v<T>);
        lua_pushinteger(l, lua_Integer(value));
    }
}


namespace detail {

std::string_view check_string_view(lua_State* l, int idx);

core::Vector<void**>& all_external_objects(lua_State* l);


template<typename T>
struct UData : NonMovable {
    T* ptr = nullptr;
    std::unique_ptr<T> storage;

    bool is_owner() const {
        return storage != nullptr;
    }
};

static_assert(offsetof(UData<int>, ptr) == 0);

template<typename T>
static inline UData<T>* check_udata(lua_State* l, int idx) {
    return static_cast<UData<T>*>(luaL_checkudata(l, idx, T::_y_reflect_type_name));
}

template<typename T>
static inline void get_arg_value(lua_State* l, T& value, int idx) {
    static_assert(can_get_value_v<T>, "Unable to get value");

    if constexpr(std::is_same_v<T, core::String>) {
        value = check_string_view(l, idx);
    } else if constexpr(std::is_floating_point_v<T>) {
        value = T(luaL_checknumber(l, idx));
    } else {
        static_assert(std::is_integral_v<T>);
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
static inline Tpl collect_args(lua_State* l) {
    Tpl args = {};
    collect_args_internal<0>(l, args);
    return args;
}

template<typename T>
static inline T default_ctor() {
    return T{};
}

template<typename T>
void reg_create_func_for_type() {}
}


void clean_external_objects(lua_State* l);


template<typename T>
using CreateObjectFunc = T (*)(lua_State*);

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

template<typename T>
void create_type_metatable_internal(lua_State* l, CreateObjectFunc<T> create_func) {
    static_assert(reflect::has_reflect_v<T>);

    using UData = detail::UData<T>;

    auto get = [](lua_State* l) -> int {
        const std::string_view name = detail::check_string_view(l, -1);
        const T* ptr = detail::check_udata<T>(l, -2)->ptr;

        if(!ptr) {
            return 0;
        }

        int ret = 0;
        reflect::explore_members<T>([&](std::string_view member_name, auto member) {
            if constexpr(can_push_value_v<remove_cvref_t<decltype(ptr->*member)>>) {
                if(member_name == name) {
                    y_debug_assert(!ret);
                    ret = 1;
                    push_value(l, ptr->*member);
                }
            }
        });

        if(!ret) {
            lua_getmetatable(l, -2);
            lua_pushstring(l, name.data());
            lua_rawget(l, -2);
            return 1;
        }

        return ret;
    };

    auto set = [](lua_State* l) -> int {
        const std::string_view name = detail::check_string_view(l, -2);
        T* ptr = detail::check_udata<T>(l, -3)->ptr;

        if(!ptr) {
            return 0;
        }

        reflect::explore_members<T>([&](std::string_view member_name, auto member) {
            if constexpr(can_get_value_v<remove_cvref_t<decltype(ptr->*member)>>) {
                if(member_name == name) {
                    detail::get_arg_value(l, ptr->*member, -1);
                }
            }
        });

        return 0;
    };

    auto create = [](lua_State* l) -> int {
        lua_pushlightuserdata(l, detail::reg_create_func_for_type<T>);
        lua_rawget(l, LUA_REGISTRYINDEX);
        y_debug_assert(lua_isuserdata(l, -1));

        CreateObjectFunc<T> create_func = static_cast<CreateObjectFunc<T>>(lua_touserdata(l, -1));
        y_debug_assert(create_func);

        void* mem = lua_newuserdata(l, sizeof(UData));
        UData* udata = new(mem) UData();
        udata->storage = std::make_unique<T>(create_func(l));
        udata->ptr = udata->storage.get();

        luaL_getmetatable(l, T::_y_reflect_type_name);
        y_debug_assert(lua_istable(l, -1));

        lua_setmetatable(l, -2);

        return 1;
    };

    auto destroy = [](lua_State* l) -> int {
        UData* udata = detail::check_udata<T>(l, -1);
        y_debug_assert(udata);

        if(udata->ptr && !udata->is_owner()) {
            auto& externals = detail::all_external_objects(l);
            const auto it = std::find(externals.begin(), externals.end(), reinterpret_cast<void**>(&udata->ptr));
            y_debug_assert(it != externals.end());
            externals.erase_unordered(it);
        }

        std::destroy_at(udata);

        return 0;
    };

    auto is_stale = [](lua_State* l) -> int {
        const UData* udata = detail::check_udata<T>(l, -1);
        lua_pushboolean(l, !udata || !udata->ptr);
        return 1;
    };

    auto to_string = [](lua_State* l) -> int {
        const UData* udata = detail::check_udata<T>(l, -1);
        lua_pushfstring(l, "%s: %c%p", T::_y_reflect_type_name, udata->is_owner() ? '#' : '@', udata->ptr);
        return 1;
    };

    luaL_newmetatable(l, T::_y_reflect_type_name);

    lua_pushvalue(l, -1);
    lua_setglobal(l, T::_y_reflect_type_name);

    lua_pushcfunction(l, get);
    lua_setfield(l, -2, "__index");

    lua_pushcfunction(l, set);
    lua_setfield(l, -2, "__newindex");

    lua_pushcfunction(l, destroy);
    lua_setfield(l, -2, "__gc");

    lua_pushcfunction(l, to_string);
    lua_setfield(l, -2, "__tostring");

    lua_pushcfunction(l, is_stale);
    lua_setfield(l, -2, "is_stale");

    lua_pushstring(l, T::_y_reflect_type_name);
    lua_setfield(l, -2, "__typename");

    if(create_func) {
        lua_pushlightuserdata(l, detail::reg_create_func_for_type<T>);
        lua_pushlightuserdata(l, create_func);
        lua_rawset(l, LUA_REGISTRYINDEX);

        lua_pushcfunction(l, create);
        lua_setfield(l, -2, "new");
    }
}

template<typename T, auto Ctor = detail::default_ctor<T>>
void create_type_metatable(lua_State* l) {
    create_type_metatable_internal<T>(l, [](lua_State* l) {
        using traits = function_traits<decltype(Ctor)>;
        return std::apply(Ctor, detail::collect_args<traits::argument_pack>(l));
    });
}

template<typename T>
void push_object_ptr(lua_State* l, T* object) {
    using UData = detail::UData<T>;
    void* mem = lua_newuserdata(l, sizeof(UData));
    UData* udata = new(mem) UData();
    udata->ptr = object;

    detail::all_external_objects(l).push_back(reinterpret_cast<void**>(&udata->ptr));

    luaL_getmetatable(l, T::_y_reflect_type_name);
    y_always_assert(lua_istable(l, -1), "Object type has not be registered");

    lua_setmetatable(l, -2);
}

}
}
}

#endif // YAVE_SCRIPT_LUA_HELPERS_H
