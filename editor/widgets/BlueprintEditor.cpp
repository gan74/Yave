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

#include "BlueprintEditor.h"

#include <yave/blueprints/BlueprintNodeBuilder.h>

#include <y/utils/hash.h>

#include <external/imgui/imgui_internal.h>
#include <external/imgui-node-editor/imgui_node_editor.h>

namespace editor {

namespace ed = ax::NodeEditor;

static constexpr float pin_icon_size = 24.0f;
static constexpr float pin_column_gap = 32.0f;
static constexpr ImVec4 node_padding = ImVec4(8.0f, 4.0f, 8.0f, 8.0f);

// Layout / look adapted from thedmd/imgui-node-editor blueprints example.
static ImColor pin_type_color(BlueprintParamTypeIndex type) {
    if(type == blueprint_param_type_index<float>()) {
        return ImColor(147, 226, 74);
    }
    if(type == blueprint_param_type_index<bool>()) {
        return ImColor(220, 48, 48);
    }
    if(type == blueprint_param_type_index<i32>() || type == blueprint_param_type_index<u32>()) {
        return ImColor(68, 201, 156);
    }

    const u32 h = u32(hash(u32(type)));
    return ImColor(
        int(70 + (h & 0x7F)),
        int(70 + ((h >> 8) & 0x7F)),
        int(70 + ((h >> 16) & 0x7F))
    );
}

static ImColor node_header_color(std::string_view name) {
    // Unreal-like blue/teal headers; vary slightly per name.
    const u32 h = ct_str_hash(name);
    const int r = 40 + int(h & 0x3F);
    const int g = 90 + int((h >> 8) & 0x3F);
    const int b = 140 + int((h >> 16) & 0x3F);
    return ImColor(r, g, b);
}

static void draw_pin_icon(ImColor color, bool connected) {
    const ImVec2 size(pin_icon_size, pin_icon_size);
    if(ImGui::IsRectVisible(size)) {
        const ImVec2 a = ImGui::GetCursorScreenPos();
        const ImVec2 b = a + size;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        const ImRect rect(a, b);
        const float rect_w = rect.GetWidth();
        const ImVec2 center = rect.GetCenter();
        const float outline_scale = rect_w / 24.0f;
        const int extra_segments = int(2.0f * outline_scale);
        const ImU32 outer = color;
        const ImU32 inner = IM_COL32(32, 32, 32, 255);

        const float offset = -rect_w * 0.25f * 0.25f;
        const ImVec2 c = center + ImVec2(offset * 0.5f, 0.0f);

        if(connected) {
            draw_list->AddCircleFilled(c, 0.5f * rect_w / 2.0f, outer, 12 + extra_segments);
        } else {
            const float r = 0.5f * rect_w / 2.0f - 0.5f;
            draw_list->AddCircleFilled(c, r, inner, 12 + extra_segments);
            draw_list->AddCircle(c, r, outer, 12 + extra_segments, 2.0f * outline_scale);
        }
    }
    ImGui::Dummy(size);
}

static void draw_input_pin(ed::PinId id, std::string_view name, BlueprintParamTypeIndex type) {
    ed::BeginPin(id, ed::PinKind::Input);
    ed::PinPivotAlignment(ImVec2(0.0f, 0.5f));
    ed::PinPivotSize(ImVec2(0.0f, 0.0f));

    draw_pin_icon(pin_type_color(type), false);
    ImGui::SameLine();
    ImGui::TextUnformatted(name.data(), name.data() + name.size());

    ed::EndPin();
}

static void draw_output_pin(ed::PinId id, std::string_view name, BlueprintParamTypeIndex type) {
    ed::BeginPin(id, ed::PinKind::Output);
    ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
    ed::PinPivotSize(ImVec2(0.0f, 0.0f));

    ImGui::TextUnformatted(name.data(), name.data() + name.size());
    ImGui::SameLine();
    draw_pin_icon(pin_type_color(type), false);

    ed::EndPin();
}

static void draw_node_header(ed::NodeId node_id, const ImVec2& header_min, const ImVec2& header_max, ImColor header_color) {
    if(header_max.x <= header_min.x || header_max.y <= header_min.y) {
        return;
    }
    if(!ImGui::IsItemVisible()) {
        return;
    }

    ImDrawList* draw_list = ed::GetNodeBackgroundDrawList(node_id);
    const float alpha = ImGui::GetStyle().Alpha;
    const float half_border = ed::GetStyle().NodeBorderWidth * 0.5f;
    const ImU32 color = IM_COL32(0, 0, 0, int(255.0f * alpha)) | (ImU32(header_color) & IM_COL32(255, 255, 255, 0));

    const ImVec2 min = header_min - ImVec2(node_padding.x - half_border, node_padding.y - half_border);
    const ImVec2 max = ImVec2(header_max.x + node_padding.z - half_border, header_max.y);

    draw_list->AddRectFilled(min, max, color, ed::GetStyle().NodeRounding, ImDrawFlags_RoundCornersTop);
    draw_list->AddLine(
        ImVec2(min.x, header_max.y - 0.5f),
        ImVec2(max.x, header_max.y - 0.5f),
        IM_COL32(255, 255, 255, int(96.0f * alpha / 3.0f)),
        1.0f);
}


BlueprintEditor::BlueprintEditor(BlueprintWorkspace* ws) : WorkspaceWidget(ICON_FA_PROJECT_DIAGRAM " Blueprint Editor", ws, ImGuiWindowFlags_NoScrollbar) {

    ed::Config config;
    config.SettingsFile = nullptr;
    _context = ed::CreateEditor(&config);

    {
        ed::SetCurrentEditor(_context);
        ed::Style& style = ed::GetStyle();
        style.NodePadding = node_padding;
        style.NodeRounding = 5.0f;
        style.NodeBorderWidth = 1.0f;
        style.HoveredNodeBorderWidth = 4.0f;
        style.SelectedNodeBorderWidth = 2.0f;
        style.PinRounding = 0.0f;
        style.PinBorderWidth = 0.0f;
        style.Colors[ed::StyleColor_NodeBg] = ImColor(24, 24, 24, 220);
        style.Colors[ed::StyleColor_NodeBorder] = ImColor(40, 40, 40, 255);
        style.Colors[ed::StyleColor_SelNodeBorder] = ImColor(255, 176, 50, 255);
        style.Colors[ed::StyleColor_HovNodeBorder] = ImColor(255, 255, 255, 255);
        ed::SetCurrentEditor(nullptr);
    }

    _nodes << LambdaBlueprintNodeBuilder<>("Add")
        .add_input<float>("a")
        .add_input<float>("b")
        .add_output<float>("out")
        .build([](float a, float b, float& out) { out = a + b; })->create_node();

    _nodes << LambdaBlueprintNodeBuilder<>("Multiply")
        .add_input<float>("a")
        .add_input<float>("b")
        .add_output<float>("out")
        .build([](float a, float b, float& out) { out = a * b; })->create_node();

    _nodes << LambdaBlueprintNodeBuilder<>("Negate")
        .add_input<float>("in")
        .add_output<float>("out")
        .build([](float in, float& out) { out = -in; })->create_node();

    _nodes << LambdaBlueprintNodeBuilder<>("Const")
        .add_output<float>("value")
        .build([](float& value) { value = 1.0f; })->create_node();

    ed::SetCurrentEditor(_context);
    for(usize i = 0; i != _nodes.size(); ++i) {
        ed::SetNodePosition(ed::NodeId((i + 1) * 1000), ImVec2(40.0f + float(i) * 240.0f, 40.0f));
    }
    ed::SetCurrentEditor(nullptr);
}

BlueprintEditor::~BlueprintEditor() {
    ed::DestroyEditor(_context);
}

void BlueprintEditor::on_gui() {
    ed::SetCurrentEditor(_context);
    ed::Begin("##blueprint", ImGui::GetContentRegionAvail());

    for(usize i = 0; i != _nodes.size(); ++i) {
        draw_node(*_nodes[i], i);
    }

    ed::End();
    ed::SetCurrentEditor(nullptr);
}

void BlueprintEditor::draw_node(const BlueprintNode& node, usize index) {
    const uintptr_t base = (index + 1) * 1000;
    const ed::NodeId node_id(base);
    const ImColor header_color = node_header_color(node.name());

    ed::PushStyleVar(ed::StyleVar_NodePadding, node_padding);
    ed::BeginNode(node_id);
    ImGui::PushID(int(base));

    ImGui::BeginGroup();
    {
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
        ImGui::TextUnformatted(node.name().data(), node.name().data() + node.name().size());
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
    }
    ImGui::EndGroup();
    const ImVec2 header_min = ImGui::GetItemRectMin();
    const ImVec2 header_max = ImGui::GetItemRectMax();

    ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y));

    ImGui::BeginGroup();
    for(usize p = 0; p != node.input_count(); ++p) {
        draw_input_pin(ed::PinId(base + 1 + p), node.input_name(p), node.input_type(p));
    }
    if(!node.input_count()) {
        ImGui::Dummy(ImVec2(pin_icon_size, pin_icon_size));
    }
    ImGui::EndGroup();
    const float inputs_max_x = ImGui::GetItemRectMax().x;

    ImGui::SameLine(0.0f, pin_column_gap);

    ImGui::BeginGroup();
    for(usize p = 0; p != node.output_count(); ++p) {
        draw_output_pin(ed::PinId(base + 500 + p), node.output_name(p), node.output_type(p));
    }
    if(!node.output_count()) {
        ImGui::Dummy(ImVec2(pin_icon_size, pin_icon_size));
    }
    ImGui::EndGroup();
    const float outputs_max_x = ImGui::GetItemRectMax().x;

    // Stretch header across the full node width (title alone may be narrower).
    const ImVec2 full_header_max(std::max(header_max.x, std::max(inputs_max_x, outputs_max_x)), header_max.y);

    ImGui::PopID();
    ed::EndNode();
    ed::PopStyleVar();

    draw_node_header(node_id, header_min, full_header_max, header_color);
}

}
