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

#include "helpers.h"

namespace yave {
namespace as {

void register_string(asIScriptEngine* engine, TypeNameMap& map) {
    bind_type<core::String>(engine, map, "string");

    //r = engine->RegisterStringFactory("string", GetStdStringFactorySingleton());
    check(engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f(uint8 c)", asFUNCTION(+[](const char c, core::String* self) {
        new(self) core::String(&c, 1);
    }), asCALL_CDECL_OBJLAST));

    check(engine->RegisterObjectMethod("string", "string &opAddAssign(const string &in)", asFUNCTION(+[](const core::String& str, core::String& dst) -> core::String& {
        return dst += str;
    }), asCALL_CDECL_OBJLAST));

    check(engine->RegisterObjectMethod("string", "bool opEquals(const string &in) const", asFUNCTION(+[](const core::String& a, const core::String& b) -> bool {
        return a == b;
    }), asCALL_CDECL_OBJLAST));

    check(engine->RegisterObjectMethod("string", "string opAdd(const string &in) const", asFUNCTION(+[](const core::String& a, const core::String& self) -> core::String {
        return self + a;
    }), asCALL_CDECL_OBJLAST));

    bind_method<&core::String::size>(engine, map, "length");
    bind_method<&core::String::is_empty>(engine, map, "is_empty");
    bind_method<resolve_const<>(&core::String::operator[])>(engine, map, "opIndex");
}

}
}


