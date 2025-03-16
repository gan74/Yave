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

#include <external/ImGuiColorTextEdit/TextEditor.h>

namespace editor {

static const core::String script_file = "script.as";

ScriptingConsole::ScriptingConsole() : Widget(ICON_FA_CODE " Scripting Console", ImGuiWindowFlags_NoNavInputs), _editor(std::make_unique<TextEditor>()) {
    _editor->SetLanguageDefinition(TextEditor::LanguageDefinitionId::AngelScript);
    _editor->SetPalette(TextEditor::PaletteId::Dark);

    if(auto f = io2::File::open(script_file)) {
        std::string str;
        str.resize(f.unwrap().size());
        f.unwrap().read_array(str.data(), str.size()).ignore();
        _editor->SetText(str);
    }
}

ScriptingConsole::~ScriptingConsole() {
    if(auto f = io2::File::create(script_file)) {
        const std::string code = _editor->GetText();
        f.unwrap().write_array(code.data(), code.size()).ignore();
    }
}

void ScriptingConsole::on_gui() {
    const float height = ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 1.5f;
    _editor->Render("##editor", ImGui::IsWindowFocused(), ImVec2(-1.0f, height));

    if(ImGui::Button(ICON_FA_PLAY " Run")) {
        run(_editor->GetText());
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

