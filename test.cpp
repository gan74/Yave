#include <cstdio>

#include "external/LuaJIT/src/lua.hpp"

#include <y/utils.h>
#include <y/utils/format.h>
#include <y/reflect/reflect.h>
#include <y/core/HashMap.h>
#include <y/utils/log.h>

using namespace y;

[[maybe_unused]]
static void lua_check(int err) {
    y_always_assert(err == LUA_OK, "Lua error");
}

struct MetaTest {
    int foo = 4;
    int blap = 904;

    y_reflect(MetaTest, foo, blap);

};

template<typename T>
void create_relf_metatable() {
    reflect::explore_members<T>([](std::string_view name, auto member) {
        log_msg(fmt("% : %", name, ct_type_name<decltype(std::declval<T>().*member)>()));
    });
}

int main(int, char**) {
    lua_State* l = lua_open();
    luaL_openlibs(l);

    create_relf_metatable<MetaTest>();

    const char* code = R"#(print('Hello Lua'))#";

    if(luaL_dostring(l, code) != LUA_OK) {
        printf("%s\n", lua_tostring(l, lua_gettop(l)));
    }
    lua_pop(l, lua_gettop(l));

    lua_close(l);
    return 0;
}
