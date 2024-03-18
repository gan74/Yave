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

#include <y/io2/File.h>

#include <y/utils/format.h>

#include <external/wren/src/include/wren.hpp>


namespace editor {

static const core::String script_file = "script.wren";

ScriptingConsole::ScriptingConsole() : Widget(ICON_FA_CODE " Scripting Console") {
    if(auto f = io2::File::open(script_file)) {
        _code.resize(f.unwrap().size());
        f.unwrap().read_array(_code.data(), _code.size()).ignore();
    }
}

void ScriptingConsole::on_gui() {
    const float height = ImGui::GetContentRegionAvail().y * 0.5f - ImGui::GetTextLineHeightWithSpacing();

    ImGui::SetNextItemWidth(-1);
    imgui::text_input_multiline("##log", _log, {0.0f, height}, ImGuiInputTextFlags_ReadOnly);

    ImGui::SetNextItemWidth(-1);
    imgui::text_input_multiline("##code", _code, {0.0f, height});

    ImGui::SetNextItemWidth(-1);
    if(ImGui::Button(ICON_FA_PLAY " Run")) {
        _log += "\n";
        _log += _code;

        run(_code);

        if(auto f = io2::File::create(script_file)) {
            f.unwrap().write_array(_code.data(), _code.size()).ignore();
        }
    }
}

void ScriptingConsole::run(const core::String& code) {
    WrenConfiguration config = {};
    {
        wrenInitConfiguration(&config);
        config.userData = this;
        config.writeFn = [](WrenVM* vm, const char* text) {
            ScriptingConsole* console = static_cast<ScriptingConsole*>(wrenGetUserData(vm));
            console->_log += "\n";
            console->_log += text;
        };
        config.errorFn = [](WrenVM* vm, WrenErrorType errorType, const char*, const int line, const char* msg) {
            ScriptingConsole* console = static_cast<ScriptingConsole*>(wrenGetUserData(vm));
            console->_log += "\n";
            switch (errorType) {
                case WREN_ERROR_COMPILE:
                    console->_log += "[Compile error] ";
                break;

                case WREN_ERROR_STACK_TRACE:
                    console->_log += "[Error] ";
                break;

                case WREN_ERROR_RUNTIME:
                    console->_log += "[Runtime error] ";
                break;
            }
            console->_log += fmt("{} at line {}", msg, line);
        };
    }

    WrenVM* vm = wrenNewVM(&config);
    y_defer(wrenFreeVM(vm));

    switch(wrenInterpret(vm, "default_module", code.data())) {
        case WREN_RESULT_COMPILE_ERROR:
        case WREN_RESULT_RUNTIME_ERROR:
        break;

        case WREN_RESULT_SUCCESS:
        break;
    }
}

}

