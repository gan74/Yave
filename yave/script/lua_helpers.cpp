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

#include "lua_helpers.h"

namespace yave {
namespace script {
namespace lua {

namespace detail {

std::string_view check_string_view(lua_State* l, int idx) {
    usize len = 0;
    const char* name_ptr = luaL_checklstring(l, idx, &len);
    return std::string_view(name_ptr, len);
}

core::Vector<void**>& all_external_objects(lua_State* l) {
    void* key = static_cast<void*>(all_external_objects);
    lua_pushlightuserdata(l, key);
    lua_rawget(l, LUA_REGISTRYINDEX);

    using T = core::Vector<void**>;
    T* ptr =  static_cast<T*>(lua_touserdata(l, -1));
    lua_pop(l, 1);

    if(!ptr) {
        lua_pushlightuserdata(l, key);

        ptr = static_cast<T*>(lua_newuserdata(l, sizeof(T)));
        new(ptr) T();

        lua_rawset(l, LUA_REGISTRYINDEX);
    }

    return *ptr;
}
}

void clean_external_objects(lua_State* l) {
    auto& externals = detail::all_external_objects(l);
    for(void** ptr : externals) {
        *ptr = nullptr;
    }
    externals.clear();
}

}
}
}
