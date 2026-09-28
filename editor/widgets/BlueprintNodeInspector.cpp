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

template<typename T>
static void draw_output(std::string_view label, const T& value) {
    ImGui::TextUnformatted(fmt_c_str("{}: {}", label, value));
}

template<typename T, typename M, typename I, typename O>
static void add_drawer(M& drawers, I input, O output) {
    drawers[blueprint_param_type_index<T>()] = {
        [=](std::string_view label, void* value) { input(label, *static_cast<T*>(value)); },
        [=](std::string_view label, const void* value) { output(label, *static_cast<const T*>(value)); }
    };
}

BlueprintNodeInspector::BlueprintNodeInspector(BlueprintWorkspace* ws) : WorkspaceWidget(ICON_FA_WRENCH " Blueprint Node Inspector", ws) {
    add_drawer<float>(_drawers, [](std::string_view label, float& value) {
        ImGui::DragFloat(fmt_c_str("{}", label), &value, 0.1f);
    }, draw_output<float>);

    add_drawer<math::Vec2>(_drawers, [](std::string_view label, math::Vec2& value) {
        ImGui::DragFloat2(fmt_c_str("{}", label), value.data(), 0.1f);
    }, draw_output<math::Vec2>);

    add_drawer<math::Vec3>(_drawers, [](std::string_view label, math::Vec3& value) {
        ImGui::DragFloat3(fmt_c_str("{}", label), value.data(), 0.1f);
    }, draw_output<math::Vec3>);

    add_drawer<math::Vec4>(_drawers, [](std::string_view label, math::Vec4& value) {
        ImGui::DragFloat4(fmt_c_str("{}", label), value.data(), 0.1f);
    }, draw_output<math::Vec4>);

    add_drawer<bool>(_drawers, [](std::string_view label, bool& value) {
        ImGui::Checkbox(fmt_c_str("{}", label), &value);
    }, draw_output<bool>);
}

void BlueprintNodeInspector::on_gui() {
    BlueprintNode* node = workspace()->selected_node();
    if(!node) {
        ImGui::TextDisabled("No node selected");
        return;
    }

    ImGui::TextUnformatted(node->node_type_name().data());

    imgui::text_input("##name", node->name());
    ImGui::Separator();

    const core::Span<BlueprintPin> inputs = node->input_pins();
    if(!inputs.is_empty() && ImGui::CollapsingHeader("Inputs", ImGuiTreeNodeFlags_DefaultOpen)) {
        for(usize i = 0; i != inputs.size(); ++i) {
            const void* linked = node->input(i);
            const std::string_view label = fmt("{}{}", inputs[i].name, linked ? " (linked)" : "");

            if(const auto it = _drawers.find(inputs[i].type); it != _drawers.end()) {
                it->second.input(label, node->default_input(i));
                if(linked) {
                    it->second.output("received", linked);
                }
            } else {
                ImGui::TextUnformatted(label.data());
            }
        }
    }

    const core::Span<BlueprintPin> params = node->param_pins();
    if(!params.is_empty() && ImGui::CollapsingHeader("Params", ImGuiTreeNodeFlags_DefaultOpen)) {
        for(usize i = 0; i != params.size(); ++i) {
            const std::string_view name = params[i].name;
            if(const auto it = _drawers.find(params[i].type); it != _drawers.end()) {
                it->second.input(fmt("{}##param", name), node->param_ptr(i));
            } else {
                ImGui::TextUnformatted(name.data());
            }
        }
    }

    const core::Span<BlueprintPin> outputs = node->output_pins();
    if(!outputs.is_empty() && ImGui::CollapsingHeader("Outputs", ImGuiTreeNodeFlags_DefaultOpen)) {
        for(usize i = 0; i != outputs.size(); ++i) {
            const std::string_view name = outputs[i].name;
            if(const auto it = _drawers.find(outputs[i].type); it != _drawers.end()) {
                it->second.output(name, node->output_ptr(i));
            } else {
                ImGui::TextUnformatted(name.data());
            }
        }
    }
}

}
