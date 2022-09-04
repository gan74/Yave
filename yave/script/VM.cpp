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
#include "VM.h"

#include <y/core/HashMap.h>
#include <y/core/String.h>

#include <y/reflect/reflect.h>

#include <y/utils.h>
#include <y/utils/format.h>
#include <y/utils/log.h>

namespace yave {
namespace script {

VM VM::create() {
    y_profile();

    VM vm;

    vm._l = luaL_newstate();
    // vm._l = lua_newstate(nullptr, nullptr);

    luaL_openlibs(vm._l);

    return vm;
}

VM::~VM() {
    if(_l) {
        lua_close(_l);
    }
}

VM::VM(VM&& other) {
    swap(other);
}

VM& VM::operator=(VM&& other) {
    swap(other);
    return *this;
}

void VM::swap(VM& other) {
    std::swap(_l, other._l);
}

void VM::gc() {
    lua_gc(_l, LUA_GCCOLLECT, 0);
}

core::Result<void, VM::Error> VM::run(const char* code) {
    y_defer(lua_pop(_l, lua_gettop(_l)));

    if(luaL_dostring(_l, code) != LUA_OK) {
        return core::Err(Error{core::String(lua_tostring(_l, lua_gettop(_l)))});
    }
    return core::Ok();
}


}
}
