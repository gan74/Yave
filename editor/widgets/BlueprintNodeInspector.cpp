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

static void draw_unsupported(std::string_view label, const BlueprintParamType* type) {
    const std::string_view type_name = type ? type->name : "no type";
    ImGui::TextDisabled("%s", fmt_c_str("{}: <{}, no editor>", label, type_name));
}

template<typename T, typename M, typename F>
static void add_drawer(M& drawers, F draw) {
    drawers[blueprint_param_type<T>()] = [=](std::string_view label, void* value) { draw(label, *static_cast<T*>(value)); };
}





BlueprintNodeInspector::BlueprintNodeInspector(BlueprintWorkspace* ws) : WorkspaceWidget(ICON_FA_WRENCH " Blueprint Node Inspector", ws) {
    add_drawer<float>(_drawers, [](std::string_view label, float& value) {
        ImGui::DragFloat(fmt_c_str("{}", label), &value, 0.1f);
    });

    add_drawer<math::Vec2>(_drawers, [](std::string_view label, math::Vec2& value) {
        ImGui::DragFloat2(fmt_c_str("{}", label), value.data(), 0.1f);
    });

    add_drawer<math::Vec3>(_drawers, [](std::string_view label, math::Vec3& value) {
        ImGui::DragFloat3(fmt_c_str("{}", label), value.data(), 0.1f);
    });

    add_drawer<math::Vec4>(_drawers, [](std::string_view label, math::Vec4& value) {
        ImGui::DragFloat4(fmt_c_str("{}", label), value.data(), 0.1f);
    });

    add_drawer<i32>(_drawers, [](std::string_view label, i32& value) {
        ImGui::DragScalar(fmt_c_str("{}", label), ImGuiDataType_S32, &value, 0.1f);
    });

    add_drawer<u32>(_drawers, [](std::string_view label, u32& value) {
        ImGui::DragScalar(fmt_c_str("{}", label), ImGuiDataType_U32, &value, 0.1f);
    });

    add_drawer<bool>(_drawers, [](std::string_view label, bool& value) {
        ImGui::Checkbox(fmt_c_str("{}", label), &value);
    });
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

    const Blueprint& blueprint = workspace()->blueprint();

    const core::Span<BlueprintPin> inputs = node->input_pins();
    if(!inputs.is_empty() && ImGui::CollapsingHeader("Inputs", ImGuiTreeNodeFlags_DefaultOpen)) {
        for(usize i = 0; i != inputs.size(); ++i) {
            const std::string_view label = fmt("{}{}", inputs[i].name, blueprint.find_link(node, i) ? " (linked)" : "");
            if(const auto it = _drawers.find(inputs[i].type); it != _drawers.end()) {
                it->second(label, node->default_input(i));
            } else {
                draw_unsupported(label, inputs[i].type);
            }
        }
    }

    const core::Span params = node->param_pins();
    if(!params.is_empty() && ImGui::CollapsingHeader("Params", ImGuiTreeNodeFlags_DefaultOpen)) {
        for(usize i = 0; i != params.size(); ++i) {
            const std::string_view name = params[i].name;
            if(const auto it = _drawers.find(params[i].type); it != _drawers.end()) {
                it->second(fmt("{}##param", name), node->param_ptr(i));
            } else {
                draw_unsupported(name, params[i].type);
            }
        }
    }
}

}
