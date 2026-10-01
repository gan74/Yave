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

static void draw_unsupported(std::string_view label, const BlueprintParamType* type, const char* reason = nullptr) {
    const std::string_view type_name = type ? type->name : "no type";
    ImGui::TextDisabled("%s", fmt_c_str("{}: <{}{}{}>", label, type_name, reason ? ", " : "", reason ? reason : ""));
}

template<typename T, typename M, typename I, typename O>
static void add_drawer(M& drawers, I input, O output) {
    drawers[blueprint_param_type<T>()] = {
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

    add_drawer<i32>(_drawers, [](std::string_view label, i32& value) {
        ImGui::DragScalar(fmt_c_str("{}", label), ImGuiDataType_S32, &value, 0.1f);
    }, draw_output<i32>);

    add_drawer<u32>(_drawers, [](std::string_view label, u32& value) {
        ImGui::DragScalar(fmt_c_str("{}", label), ImGuiDataType_U32, &value, 0.1f);
    }, draw_output<u32>);

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

    const Blueprint& blueprint = workspace()->blueprint();
    const usize node_index = blueprint.find_node_index(node);
    const BlueprintInstance* instance = workspace()->instance();

    const core::Span<BlueprintPin> inputs = node->input_pins();
    if(!inputs.is_empty() && ImGui::CollapsingHeader("Inputs", ImGuiTreeNodeFlags_DefaultOpen)) {
        for(usize i = 0; i != inputs.size(); ++i) {
            const BlueprintLink* link = blueprint.find_link(node_index, i);
            const std::string_view label = fmt("{}{}", inputs[i].name, link ? " (linked)" : "");

            if(const auto it = _drawers.find(inputs[i].type); it != _drawers.end()) {
                it->second.input(label, node->default_input(i));
                if(link && instance) {
                    if(const void* received = instance->all_nodes()[link->src_node]->output_ptr(link->src_pin)) {
                        it->second.output("received", received);
                    }
                }
            } else {
                draw_unsupported(label, inputs[i].type, "no editor");
            }
        }
    }

    const core::Span params = node->param_pins();
    if(!params.is_empty() && ImGui::CollapsingHeader("Params", ImGuiTreeNodeFlags_DefaultOpen)) {
        for(usize i = 0; i != params.size(); ++i) {
            const std::string_view name = params[i].name;
            if(const auto it = _drawers.find(params[i].type); it != _drawers.end()) {
                it->second.input(fmt("{}##param", name), node->param_ptr(i));
            } else {
                draw_unsupported(name, params[i].type, "no editor");
            }
        }
    }

    const core::Span outputs = node->output_pins();
    if(!outputs.is_empty() && ImGui::CollapsingHeader("Outputs", ImGuiTreeNodeFlags_DefaultOpen)) {
        for(usize i = 0; i != outputs.size(); ++i) {
            const std::string_view name = outputs[i].name;
            const void* output = instance ? instance->all_nodes()[node_index]->output_ptr(i) : nullptr;
            const auto it = _drawers.find(outputs[i].type);
            if(it != _drawers.end() && output) {
                it->second.output(name, output);
            } else if(it == _drawers.end()) {
                draw_unsupported(name, outputs[i].type, "can't display");
            } else {
                draw_unsupported(name, outputs[i].type, "not evaluated");
            }
        }
    }
}

}
