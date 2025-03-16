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

#include <external/angelscript/add_on/scriptstdstring/scriptstdstring.h>
#include <external/angelscript/add_on/scriptbuilder/scriptbuilder.h>


namespace yave {

static void message_callback(const asSMessageInfo* msg, void* out) {
    const std::string_view formatted_msg = fmt("{} ({}, {}): {}", msg->section, msg->row, msg->col, msg->message);
    if(out) {
        *static_cast<core::String*>(out) += formatted_msg;
    }
    switch(msg->type) {
        case asMSGTYPE_ERROR:
            log_msg(formatted_msg, Log::Error);
        break;

        case asMSGTYPE_WARNING:
            log_msg(formatted_msg, Log::Warning);
        break;

        default:
            log_msg(formatted_msg);
    }
}

struct TestType {
    static inline usize init = 0;

    TestType() {
        val = init++;
        log_msg(fmt("+ TestType({})", val));
    }

    TestType(const TestType&) : TestType() {
    }

    ~TestType() {
        log_msg(fmt("- TestType({})", val));
        val = usize(-1);
    }

    TestType& operator=(const TestType&) {
        val = ++init;
        log_msg(fmt("TestType = {}", val));
        return *this;
    }

    void method(int i) {
        log_msg(fmt("method({})", i));
    }

    usize val = usize(-1);
};

TestType test_func(const TestType& i) {
    return i;
}


ScriptVM::ScriptVM() {
    _engine = asCreateScriptEngine();
    y_debug_assert(_engine);

    RegisterStdString(_engine);

    _engine->SetMessageCallback(asFUNCTION(message_callback), &_output, asCALL_CDECL);

    as::check(_engine->RegisterGlobalFunction("void print(const string& in)", asFUNCTION(+[](const std::string& str) { log_msg(str); }), asCALL_CDECL), "Unable to register function");

    as::bind_type<TestType>(_engine, _types, "TestType");
    as::bind_global_func(_engine, _types, "test_func", test_func);

    as::bind_method<&TestType::method>(_engine, _types, "method");

}

ScriptVM::~ScriptVM() {
    _engine->ShutDownAndRelease();
}

core::Result<void, core::String> ScriptVM::run(const core::String& script, const char* section_name) {
    _output = {};

    CScriptBuilder builder;
    if(as::is_error(builder.StartNewModule(_engine, "Module")) ||
       as::is_error(builder.AddSectionFromMemory(section_name, script.data(), unsigned(script.size()))) ||
       as::is_error(builder.BuildModule())) {
        return core::Err(_output);
    }

    asIScriptModule* mod = _engine->GetModule("Module");
    asIScriptFunction* func = mod->GetFunctionByDecl("void main()");
    if(!func) {
        return core::Err(core::String("The script must have the function 'void main()'. Please add it and try again"));
    }

    asIScriptContext* ctx = _engine->CreateContext();
    ctx->Prepare(func);
    y_defer(ctx->Release());

    switch(ctx->Execute()) {
        case asEXECUTION_FINISHED:
            return core::Ok();

        case asEXECUTION_EXCEPTION:
            return core::Err(fmt_to_owned("An exception '{}' occurred", ctx->GetExceptionString()));

        default:
            y_fatal("???");
    }

    return core::Ok();
}

}

