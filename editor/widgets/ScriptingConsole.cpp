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

static const core::String script_file = "script.lua";

ScriptingConsole::ScriptingConsole() : Widget(ICON_FA_CODE " Scripting Console", ImGuiWindowFlags_NoNavInputs) {
    if(auto f = io2::File::open(script_file)) {
        _code.resize(f.unwrap().size());
        f.unwrap().read_array(_code.data(), _code.size()).ignore();
    }
}

void ScriptingConsole::on_gui() {
    const float height = ImGui::GetContentRegionAvail().y * 0.5f - ImGui::GetTextLineHeightWithSpacing();

    ImGui::SetNextItemWidth(-1);
    imgui::text_input_multiline("##code", _code, {0.0f, height});

    ImGui::SetNextItemWidth(-1);
    ImGui::PushStyleColor(ImGuiCol_Text, imgui::error_text_color);
    imgui::text_input_multiline("##error", _error, {0.0f, height}, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleColor();

    if(ImGui::Button(ICON_FA_PLAY " Run")) {
        if(auto f = io2::File::create(script_file)) {
            f.unwrap().write_array(_code.data(), _code.size()).ignore();
        }

        run(_code);
    }
}


void ScriptingConsole::run(const core::String& code) {
    ScriptVM vm;

    _error = {};
    if(auto r = vm.run(code); r.is_error()) {
        _error = r.error();
    }
}

}

