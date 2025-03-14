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

#include "ScriptVM.h"
#include "lua_helpers.h"

#include <y/core/ScratchPad.h>

#include <y/utils/log.h>
#include <y/utils/format.h>


namespace yave {
namespace detail {

ScriptTypeIndex next_script_type_index() {
    static std::atomic<std::underlying_type_t<ScriptTypeIndex>> global_type_index = 0;
    return ScriptTypeIndex(global_type_index++);
}
}


struct LuaUserData {
    LuaUserData* self = nullptr;
    void (*dtor)(LuaUserData*) = nullptr;
};


static float test_func(float x) {
    return x + 1;
}


static int api_tostring(lua_State* L) {
    LuaUserData* userdata = static_cast<LuaUserData*>(luaL_checkudata(L, 1, "Vector"));
    y_debug_assert(userdata->self == userdata);
    lua_pushfstring(L, fmt_c_str("userdata({})", static_cast<void*>(userdata)));
    return 1;
}

static int api_dtor(lua_State* L) {
    LuaUserData* userdata = static_cast<LuaUserData*>(luaL_checkudata(L, 1, "Vector"));
    y_debug_assert(userdata->self == userdata);
    userdata->dtor(userdata);
    return 0;
}

static int api_new(lua_State *L) {
    LuaUserData* userdata = static_cast<LuaUserData*>(lua_newuserdata(L, sizeof(LuaUserData)));
    userdata->self = userdata;
    userdata->dtor = [](LuaUserData* data) {
        log_msg(fmt("destroying {}", static_cast<void*>(data)));
    };

    luaL_getmetatable(L, "Vector");
    lua_setmetatable(L, -2);
    return 1;
}

void setup_object(lua_State* L) {
    static const struct luaL_Reg vector_methods[] = {
        {"__tostring", api_tostring},
        {"__gc", api_dtor},
        {nullptr, nullptr}
    };

    luaL_newmetatable(L, "Vector");
    luaL_setfuncs(L, vector_methods, 0);

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    lua_newtable(L);
    const int pos = lua_gettop(L);

    lua_pushcfunction(L, api_new);
    lua_setfield(L, pos, "new");
    lua_setglobal(L, "Vector");
}




ScriptVM::ScriptVM() {
    _state = luaL_newstate();
    luaL_openlibs(_state);

    // push_user_data(_state, this);
    // lua_setglobal(_state, "_vm");

    lua::bind_func<test_func>(_state, "test");

    setup_object(_state);
}

ScriptVM::~ScriptVM() {
    lua_close(_state);
}

core::Result<void, core::String> ScriptVM::run(const core::String& script) {
    y_defer(lua_pop(_state, lua_gettop(_state)));
    if(luaL_loadstring(_state, script.data()) == LUA_OK) {
        if(lua_pcall(_state, 0, 0, 0) == LUA_OK) {
            return core::Ok();
        }
    }

    core::String err = lua_tostring(_state, lua_gettop(_state));
    return core::Err(std::move(err));
}

}

