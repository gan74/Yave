/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

#include "BlueprintErrorConsole.h"
#include "BlueprintEditor.h"

#include <editor/UiManager.h>

#include <editor/utils/ui.h>

#include <y/utils/format.h>

namespace editor {

BlueprintErrorConsole::BlueprintErrorConsole(BlueprintWorkspace* ws) :
        WorkspaceWidget(ICON_FA_EXCLAMATION_TRIANGLE " Blueprint Errors", ws) {
}

void BlueprintErrorConsole::on_gui() {
    const BlueprintNode* error_node = workspace()->error_node();
    if(!error_node) {
        ImGui::TextDisabled("No errors");
        return;
    }

    const BlueprintError& error = workspace()->error().error();

    ImGui::PushStyleColor(ImGuiCol_Text, imgui::error_text_color);
    const bool clicked = ImGui::Selectable(fmt_c_str("Node \"{}\": {}", error_node->name(), error.error));
    ImGui::PopStyleColor();

    if(clicked) {
        for(const auto& widget : ui().top_level_widgets()) {
            if(auto* editor = dynamic_cast<BlueprintEditor*>(widget.get())) {
                if(editor->workspace() == workspace()) {
                    editor->center_on_node(error_node);
                    break;
                }
            }
        }
    }
}

}
