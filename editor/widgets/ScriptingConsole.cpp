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

#include "ScriptingConsole.h"

#include <editor/utils/ui.h>

#include <y/io2/File.h>
#include <y/utils/format.h>

#include <algorithm>

namespace editor {

static const core::String script_file = "script.as";

ScriptingConsole::ScriptingConsole() : Widget(ICON_FA_CODE " Scripting Console", ImGuiWindowFlags_NoNavInputs) {
    if(auto f = io2::File::open(script_file)) {
        _code.resize(f.unwrap().size(), '\0');
        f.unwrap().read_array(_code.data(), _code.size()).ignore();
    }
}

ScriptingConsole::~ScriptingConsole() {
}

void ScriptingConsole::on_gui() {
    const float height = ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 1.5f;

    {
        auto callback = [](ImGuiInputTextCallbackData* data) {
           auto& str = *static_cast<core::String*>(data->UserData);

            if(data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                str.resize(usize(data->BufSize));
            }
            if(data->EventFlag == ImGuiInputTextFlags_CallbackCompletion && data->EventKey == ImGuiKey_Tab) {
                data->InsertChars(data->CursorPos, "\t");
            }
            return 0;
        };

        ImGui::InputTextMultiline("##script", _code.data(), _code.size() + 1, ImVec2(-1.0f, height), ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackCompletion, callback, &_code);
        _code.resize(std::strlen(_code.data()));
    }


    if(ImGui::Button(ICON_FA_PLAY " Run")) {
        if(auto f = io2::File::create(script_file)) {
            f.unwrap().write_array(_code.data(), _code.size()).ignore();
        }
        run(_code);
    }
}


void ScriptingConsole::run(std::string_view code) {
    ScriptVM vm;
    if(auto r = vm.run(code); r.is_error()) {
        for(const auto& e : r.error()) {
            log_msg(fmt("{} ({}, {}): {}", e.section, e.line, e.column, e.message), Log::Error);
        }
    }
}

}

