#include <cstdio>

#include "external/LuaJIT/src/lua.hpp"

#include <y/utils.h>
#include <y/utils/format.h>
#include <y/utils/log.h>

#include <y/reflect/reflect.h>
#include <y/core/HashMap.h>
#include <y/core/String.h>

using namespace y;

template<typename T>
static void push_value(lua_State* l, const T& value) {
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
static void pop_value(lua_State* l, T& value) {
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

    log_msg(fmt("%.get(%)", ptr, name));

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

    log_msg(fmt("%.set(%)", ptr, name));

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
void create_relf_metatable(lua_State* l) {
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



struct MetaTest {
    int foo = 4;
    int blap = 904;
    core::String name = "test obj";

    y_reflect(MetaTest, foo, blap, name);
};

template<typename T>
int create_obj(lua_State* l) {
    log_msg(fmt("new<%>", T::_y_reflect_type_name));
    void* ptr = static_cast<void*>(new T());
    lua_pushlightuserdata(l, ptr);

    lua_pushlightuserdata(l, registry_id<T>());
    lua_rawget(l, LUA_REGISTRYINDEX);
    y_debug_assert(lua_istable(l, -1));

    lua_setmetatable(l, -2);
    return 1;
}

int main(int, char**) {
    lua_State* l = lua_open();
    luaL_openlibs(l);

    create_relf_metatable<MetaTest>(l);

    lua_register(l, "new", create_obj<MetaTest>);

    const char* code = R"#(
        local obj = new()

        print(obj.blap)
        obj.blap = 123;
        print(obj.blap)

        print(obj.name)
        obj.name = 'new name'
        print(obj.name)
    )#";

    if(luaL_dostring(l, code) != LUA_OK) {
        printf("%s\n", lua_tostring(l, lua_gettop(l)));
    }
    lua_pop(l, lua_gettop(l));

    lua_close(l);
    return 0;
}
