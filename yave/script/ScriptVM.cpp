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

#include <y/concurrent/Mutexed.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <external/angelscript/add_on/scriptbuilder/scriptbuilder.h>

#include <unordered_set>


namespace yave {

static void message_callback(const asSMessageInfo* msg, void* out) {
    const std::string_view formatted_msg = fmt("{} ({}, {}): {}", msg->section, msg->row, msg->col, msg->message);
    if(out) {
        *static_cast<core::String*>(out) += formatted_msg;
        static_cast<core::String*>(out)->push_back('\n');
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


class ScriptStringFactory final : public asIStringFactory {
    public:
        ~ScriptStringFactory() override {
            // y_debug_assert(_cache.is_empty());
        }

        const void* GetStringConstant(const char* data, asUINT length) override {
            const std::string_view str(data, usize(length));
            return _cache.locked([str](auto& cache) {
                return &(*cache.insert(str).first);
            });
        }

        int ReleaseStringConstant(const void*) override {
            return asSUCCESS;
        }

        int GetRawStringData(const void* str, char* data, asUINT* length) const override {
            if(!str) {
                return asERROR;
            }

            const core::String& s = *static_cast<const core::String*>(str);
            if(length) {
                *length = asUINT(s.size());
            }
            if(data) {
                std::copy_n(s.data(), s.size(), data);
            }
            return asSUCCESS;
        }


    private:
        // We need pointer stability for contained strings
        concurrent::Mutexed<std::unordered_set<core::String>> _cache;
};

ScriptVM::ScriptVM() : _string_factory(std::make_unique<ScriptStringFactory>()) {

    _engine = asCreateScriptEngine();
    y_debug_assert(_engine);
    _engine->SetMessageCallback(asFUNCTION(message_callback), &_output, asCALL_CDECL);

    as::register_string(_engine, _types);
    _engine->RegisterStringFactory("string", _string_factory.get());

    as::bind_global_func(_engine, _types, "print", +[](const core::String& str) { log_msg(str); });
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
        return core::Err(core::String("The script must have the function 'void main()'"));
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

