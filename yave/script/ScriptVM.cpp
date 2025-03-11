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

#include <y/core/ScratchPad.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <external/LuaJIT/src/lua.hpp>

namespace yave {

static ScriptVM* get_vm(lua_State* L) {
    lua_getglobal(L, "_vm");
    const int top = lua_gettop(L);
    y_debug_assert(lua_islightuserdata(L, top));
    ScriptVM* vm = static_cast<ScriptVM*>(lua_touserdata(L, top));
    lua_pop(L, 1);
    return vm;
}

static int lua_print(lua_State* L) {
    ScriptVM* vm = get_vm(L);

    const int argc = lua_gettop(L);
    for (int i = 1; i <= argc; ++i) {
        usize size = 0;
        const char* str = luaL_checklstring(L, i, &size);
        vm->output += std::string_view(str, size);
    }
    vm->output += "\n";

    return 0;
}

static const luaL_Reg vm_lib[] = {
    {"print", lua_print},
    {nullptr, nullptr}
};

ScriptVM::ScriptVM() {
    _state = luaL_newstate();
    luaL_openlibs(_state);

    lua_pushlightuserdata(_state, this);
    lua_setglobal(_state, "_vm");

    lua_getglobal(_state, "_G");
    luaL_setfuncs(_state, vm_lib, 0);
    lua_pop(_state, 1);

}

ScriptVM::~ScriptVM() {
    lua_close(_state);
}

bool ScriptVM::run(const core::String& script) {
    if(luaL_loadstring(_state, script.data()) == LUA_OK) {
        if(lua_pcall(_state, 0, 0, 0) == LUA_OK) {
            lua_pop(_state, lua_gettop(_state));
        }
    }
    return true;
}

}

