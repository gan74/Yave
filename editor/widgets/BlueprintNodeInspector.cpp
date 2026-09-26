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

#include "BlueprintNodeInspector.h"

#include <editor/utils/ui.h>

#include <y/utils/format.h>

namespace editor {

BlueprintNodeInspector::BlueprintNodeInspector(BlueprintWorkspace* ws) : WorkspaceWidget(ICON_FA_WRENCH " Blueprint Node Inspector", ws) {
}

void BlueprintNodeInspector::on_gui() {
    BlueprintNode* node = workspace()->selected_node();
    if(!node) {
        ImGui::TextDisabled("No node selected");
        return;
    }

    imgui::text_read_only("##name", node->name());
    ImGui::Separator();

    if(node->input_count() && ImGui::CollapsingHeader("Inputs", ImGuiTreeNodeFlags_DefaultOpen)) {
        for(usize i = 0; i != node->input_count(); ++i) {
            const std::string_view name = node->input_name(i);
            const bool linked = node->input(i);

            if(node->input_type(i) == blueprint_param_type_index<float>()) {
                float* value = static_cast<float*>(node->default_input(i));
                ImGui::DragFloat(fmt_c_str("{}{}", name, linked ? " (linked)" : ""), value, 0.1f);
            } else {
                ImGui::TextUnformatted(fmt_c_str("{}{}", name, linked ? " (linked)" : ""));
            }
        }
    }

    if(node->output_count() && ImGui::CollapsingHeader("Outputs", ImGuiTreeNodeFlags_DefaultOpen)) {
        for(usize i = 0; i != node->output_count(); ++i) {
            const std::string_view name = node->output_name(i);
            if(node->output_type(i) == blueprint_param_type_index<float>()) {
                const float value = *static_cast<const float*>(node->output_ptr(i));
                ImGui::TextUnformatted(fmt_c_str("{}: {}", name, value));
            } else {
                ImGui::TextUnformatted(name.data(), name.data() + name.size());
            }
        }
    }
}

}
