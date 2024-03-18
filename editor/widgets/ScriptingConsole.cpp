/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include "ScriptingConsole.h"

#include <editor/utils/ui.h>

#include <y/core/HashMap.h>

#include <y/io2/File.h>
#include <y/utils/format.h>

#include <external/wren/src/include/wren.hpp>

#include <algorithm>


namespace editor {

static std::string_view error_type_name(WrenErrorType type) {
    switch (type) {
        case WREN_ERROR_COMPILE:
            return "[Compile error]";
        break;

        case WREN_ERROR_STACK_TRACE:
            return "[Error]";
        break;

        case WREN_ERROR_RUNTIME:
            return "[Runtime error]";
        break;
    }
    return "[Unknown error]";
}

static void default_print(std::string_view msg) {
    log_msg(msg);
}

static void default_error(WrenErrorType type, int line, std::string_view msg) {
    log_msg(fmt("{} at line {}: {}", error_type_name(type), line, msg), Log::Error);
}




class ScriptVM : NonCopyable {
    struct ClassKey {
        std::string_view module;
        std::string_view class_name;

        std::strong_ordering operator<=>(const ClassKey&) const = default;
        bool operator==(const ClassKey&) const = default;
        bool operator!=(const ClassKey&) const = default;
    };

    struct MethodKey {
        ClassKey class_key;
        std::string_view signature;

        std::strong_ordering operator<=>(const MethodKey&) const = default;
        bool operator==(const MethodKey&) const = default;
        bool operator!=(const MethodKey&) const = default;
    };

    struct Hasher : std::hash<std::string_view> {
        usize operator()(std::string_view s) const {
            return hash<std::string_view>::operator()(s);
        }

        usize operator()(const ClassKey& key) const {
            usize h = operator()(key.module);
            hash_combine(h, operator()(key.class_name));
            return h;
        }

        usize operator()(const MethodKey& key) const {
            usize h = operator()(key.class_key);
            hash_combine(h, operator()(key.signature));
            return h;
        }
    };

    public:
        using print_func = std::function<void(std::string_view)>;
        using error_func = std::function<void(WrenErrorType, int, std::string_view)>;

        static inline const char* module_name = "yave";

        ScriptVM(print_func print = default_print, error_func error = default_error) : _print(print), _error(error) {

            WrenConfiguration config = {};
            {
                wrenInitConfiguration(&config);
                config.userData = this;
                config.writeFn = [](WrenVM* vm, const char* msg) {
                    static_cast<ScriptVM*>(wrenGetUserData(vm))->_print(msg);
                };
                config.errorFn = [](WrenVM* vm, WrenErrorType type, const char*, const int line, const char* msg) {
                    static_cast<ScriptVM*>(wrenGetUserData(vm))->_error(type, line, msg);
                };
                config.bindForeignClassFn = [](WrenVM* vm, const char* module, const char* class_name) {
                    const ClassKey key = {module, class_name};
                    return static_cast<ScriptVM*>(wrenGetUserData(vm))->_classes[key];
                };
                config.bindForeignMethodFn = [](WrenVM* vm, const char* module, const char* class_name, bool is_static, const char* signature) {
                    const MethodKey key = {module, class_name, signature};
                    return static_cast<ScriptVM*>(wrenGetUserData(vm))->_methods[key];
                };
            }

            _vm = wrenNewVM(&config);
        }

        ~ScriptVM() {
            wrenFreeVM(_vm);
        }

        bool run(const core::String& script) {
            return wrenInterpret(_vm, module_name, script.data()) == WREN_RESULT_SUCCESS;
        }

        template<typename T>
        void bind_type() {
            const ClassKey key = {module_name, T::_y_reflect_type_name};
            _classes[key] = {
                [](WrenVM* vm) {
                    ::new(wrenSetSlotNewForeign(vm, 0, 0, sizeof(T))) T();
                },
                [](void* ptr) {
                    static_cast<T*>(ptr)->~T();
                }
            };
        }

    private:
        WrenVM* _vm = nullptr;

        core::FlatHashMap<MethodKey, WrenForeignMethodFn, Hasher> _methods;
        core::FlatHashMap<ClassKey, WrenForeignClassMethods, Hasher> _classes;

        print_func _print;
        error_func _error;
};



static const core::String script_file = "script.wren";

ScriptingConsole::ScriptingConsole() : Widget(ICON_FA_CODE " Scripting Console", ImGuiWindowFlags_NoNavInputs) {
    if(auto f = io2::File::open(script_file)) {
        _code.resize(f.unwrap().size());
        f.unwrap().read_array(_code.data(), _code.size()).ignore();
    }
}

void ScriptingConsole::on_gui() {
    const float height = ImGui::GetContentRegionAvail().y * 0.5f - ImGui::GetTextLineHeightWithSpacing();

    ImGui::SetNextItemWidth(-1);
    imgui::text_input_multiline("##log", _logs, {0.0f, height}, ImGuiInputTextFlags_ReadOnly);

    ImGui::SetNextItemWidth(-1);
    imgui::text_input_multiline("##code", _code, {0.0f, height});

    ImGui::SetNextItemWidth(-1);
    if(ImGui::Button(ICON_FA_PLAY " Run")) {
        run(_code);

        if(auto f = io2::File::create(script_file)) {
            f.unwrap().write_array(_code.data(), _code.size()).ignore();
        }
    }
}


struct Test : NonMovable {
    Test() {
        log_msg("+test");
        self = this;
    }
    ~Test() {
        log_msg("-test");
        y_debug_assert(self == this);
    }

    int x = 0;
    Test* self = nullptr;

    y_reflect(Test, x)
};

void ScriptingConsole::run(const core::String& code) {
    ScriptVM vm(
        [this](std::string_view msg) {
            _logs += msg;
            _logs += "\n";
        },
        [this](WrenErrorType type, int line, std::string_view msg) {
            _logs += fmt("{} at line {}: {}", error_type_name(type), line, msg);
            _logs += "\n";
        }
    );


    vm.bind_type<Test>();

    vm.run(code);
}

}

