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


struct TestType {
    static inline usize init = 0;

    TestType() {
        log_msg(fmt("+ TestType({})", ++init));
    }

    ~TestType() {
        log_msg(fmt("- TestType({})", --init));
    }
};

ScriptVM::ScriptVM() {
    _state = luaL_newstate();
    luaL_openlibs(_state);

    lua::bind_type<TestType>(_state, "Test");
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

