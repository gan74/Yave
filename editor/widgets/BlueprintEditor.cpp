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

#include <y/utils/hash.h>

#include <external/imgui-node-editor/imgui_node_editor.h>

#include <algorithm>

namespace editor {

namespace ed = ax::NodeEditor;

static constexpr float pin_icon_size = 16.0f;
static constexpr float pin_column_gap = 20.0f;
static constexpr ImVec4 node_padding = ImVec4(6.0f, 2.0f, 6.0f, 4.0f);

struct PinInfo {
    const BlueprintNode* node = nullptr;
    usize index = 0;
    bool is_input = false;

    BlueprintParamTypeIndex type() const {
        return is_input ? node->input_type(index) : node->output_type(index);
    }
};

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
    // Green / teal headers; vary slightly per name.
    const u32 h = ct_str_hash(name);
    const int r = 30 + int(h & 0x2F);
    const int g = 120 + int((h >> 8) & 0x4F);
    const int b = 70 + int((h >> 16) & 0x3F);
    return ImColor(r, g, b);
}

static PinInfo find_pin(const Blueprint& blueprint, ed::PinId id) {
    const uintptr_t pin = id.Get();
    for(const auto& node : blueprint.all_nodes()) {
        const uintptr_t base = uintptr_t(node.get());
        const usize in_count = node->input_count();
        const usize out_count = node->output_count();

        if(pin >= base + 1 && pin < base + 1 + in_count) {
            return {node.get(), pin - (base + 1), true};
        }

        const uintptr_t out_base = base + 1 + in_count;
        if(pin >= out_base && pin < out_base + out_count) {
            return {node.get(), pin - out_base, false};
        }
    }
    return {};
}

static void draw_pin_icon(ImColor color, bool connected) {
    const ImVec2 size(pin_icon_size, pin_icon_size);
    if(ImGui::IsRectVisible(size)) {
        const ImVec2 a = ImGui::GetCursorScreenPos();
        const ImVec2 b = a + size;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        const float rect_w = size.x;
        const ImVec2 center = (a + b) * 0.5f;
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

static void draw_input_pin(ed::PinId id, std::string_view name, BlueprintParamTypeIndex type, bool connected) {
    ed::BeginPin(id, ed::PinKind::Input);
    ed::PinPivotAlignment(ImVec2(0.0f, 0.5f));
    ed::PinPivotSize(ImVec2(0.0f, 0.0f));

    draw_pin_icon(pin_type_color(type), connected);
    ImGui::SameLine();
    ImGui::TextUnformatted(name.data(), name.data() + name.size());

    ed::EndPin();
}

static void draw_output_pin(ed::PinId id, std::string_view name, BlueprintParamTypeIndex type, bool connected) {
    ed::BeginPin(id, ed::PinKind::Output);
    ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
    ed::PinPivotSize(ImVec2(0.0f, 0.0f));

    ImGui::TextUnformatted(name.data(), name.data() + name.size());
    ImGui::SameLine();
    draw_pin_icon(pin_type_color(type), connected);

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


BlueprintEditor::BlueprintEditor(BlueprintWorkspace* ws) :
        WorkspaceWidget(ICON_FA_PROJECT_DIAGRAM " Blueprint Editor", ws, ImGuiWindowFlags_NoScrollbar) {

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
        style.SelectedNodeBorderWidth = 4.0f;
        style.PinRounding = 0.0f;
        style.PinBorderWidth = 0.0f;
        style.Colors[ed::StyleColor_Bg] = ImColor(30, 30, 30, 255);
        style.Colors[ed::StyleColor_Grid] = ImColor(255, 255, 255, 40);
        style.Colors[ed::StyleColor_NodeBg] = ImColor(24, 24, 24, 220);
        style.Colors[ed::StyleColor_NodeBorder] = ImColor(40, 40, 40, 255);
        style.Colors[ed::StyleColor_SelNodeBorder] = ImColor(255, 176, 50, 255);
        style.Colors[ed::StyleColor_HovNodeBorder] = ImColor(255, 255, 255, 255);
        ed::SetCurrentEditor(nullptr);
    }

    ed::SetCurrentEditor(_context);
    {
        const core::Span nodes = workspace()->blueprint().all_nodes();
        for(usize i = 0; i != nodes.size(); ++i) {
            ed::SetNodePosition(ed::NodeId(uintptr_t(nodes[i].get())), ImVec2(40.0f + float(i) * 240.0f, 40.0f));
        }
    }
    ed::SetCurrentEditor(nullptr);
}

BlueprintEditor::~BlueprintEditor() {
    ed::DestroyEditor(_context);
}

void BlueprintEditor::on_gui() {
    ed::SetCurrentEditor(_context);
    ed::Begin("##blueprint", ImGui::GetContentRegionAvail());

    for(const auto& node : workspace()->blueprint().all_nodes()) {
        draw_node(*node);
    }

    for(const Link& link : _links) {
        ed::Link(ed::LinkId(link.id), ed::PinId(link.start_pin), ed::PinId(link.end_pin), pin_type_color(link.type), 2.0f);
    }

    process_links();

    if(ed::HasSelectionChanged()) {
        ed::NodeId id;
        workspace()->set_selected_node(ed::GetSelectedNodes(&id, 1) == 1
            ? reinterpret_cast<BlueprintNode*>(id.Get())
            : nullptr);
    }

    ed::End();
    ed::SetCurrentEditor(nullptr);
}

bool BlueprintEditor::is_pin_linked(uintptr_t pin) const {
    return std::any_of(_links.begin(), _links.end(), [=](const Link& link) {
        return link.start_pin == pin || link.end_pin == pin;
    });
}

void BlueprintEditor::rebuild_blueprint_links() {
    y_profile();

    Blueprint& blueprint = workspace()->blueprint();
    blueprint.clear_links();
    for(const Link& link : _links) {
        const PinInfo start = find_pin(blueprint, ed::PinId(link.start_pin));
        const PinInfo end = find_pin(blueprint, ed::PinId(link.end_pin));
        y_debug_assert(start.node && !start.is_input);
        y_debug_assert(end.node && end.is_input);
        blueprint.add_link(start.node, start.index, end.node, end.index);
    }
}

void BlueprintEditor::process_links() {
    bool changed = false;

    if(ed::BeginCreate()) {
        ed::PinId start_id;
        ed::PinId end_id;
        if(ed::QueryNewLink(&start_id, &end_id) && start_id && end_id) {
            PinInfo start = find_pin(workspace()->blueprint(), start_id);
            PinInfo end = find_pin(workspace()->blueprint(), end_id);

            if(start.node && start.is_input) {
                std::swap(start, end);
                std::swap(start_id, end_id);
            }

            const bool valid =
                start.node && end.node &&
                !start.is_input && end.is_input &&
                workspace()->blueprint().is_link_valid(start.node, start.index, end.node, end.index)
            ;

            if(valid && ed::AcceptNewItem()) {
                for(usize i = 0; i != _links.size();) {
                    if(_links[i].end_pin == end_id.Get()) {
                        _links.erase_unordered(_links.begin() + i);
                    } else {
                        ++i;
                    }
                }
                _links.emplace_back(Link{_next_link_id++, start_id.Get(), end_id.Get(), start.type()});
                changed = true;
            } else if(!valid) {
                ed::RejectNewItem();
            }
        }
    }
    ed::EndCreate();

    if(ed::BeginDelete()) {
        ed::LinkId link_id;
        while(ed::QueryDeletedLink(&link_id)) {
            if(!ed::AcceptDeletedItem()) {
                continue;
            }
            const auto it = std::find_if(_links.begin(), _links.end(), [=](const Link& link) {
                return link.id == link_id.Get();
            });
            if(it != _links.end()) {
                _links.erase_unordered(it);
                changed = true;
            }
        }
    }
    ed::EndDelete();

    if(changed) {
        rebuild_blueprint_links();
    }
}

void BlueprintEditor::draw_node(const BlueprintNode& node) {
    const uintptr_t base = uintptr_t(&node);
    const ed::NodeId node_id(base);
    const ImColor header_color = node_header_color(node.name());

    ed::BeginNode(node_id);
    ImGui::PushID(int(base));

    ImGui::BeginGroup();
    ImGui::TextUnformatted(node.name().data(), node.name().data() + node.name().size());
    ImGui::EndGroup();

    const ImVec2 header_min = ImGui::GetItemRectMin();
    const ImVec2 header_max = ImGui::GetItemRectMax();

    ImGui::Dummy(ImVec2(0.0f, 2.0f));

    ImGui::BeginGroup();
    if(node.input_count()) {
        for(usize p = 0; p != node.input_count(); ++p) {
            const uintptr_t pin = base + 1 + p;
            draw_input_pin(ed::PinId(pin), node.input_name(p), node.input_type(p), is_pin_linked(pin));
        }
    } else {
        ImGui::Dummy(ImVec2(pin_icon_size, pin_icon_size));
    }
    ImGui::EndGroup();
    const float inputs_max_x = ImGui::GetItemRectMax().x;

    ImGui::SameLine(0.0f, pin_column_gap);

    ImGui::BeginGroup();
    if(node.output_count()) {
        const uintptr_t out_base = base + 1 + node.input_count();
        for(usize p = 0; p != node.output_count(); ++p) {
            const uintptr_t pin = out_base + p;
            draw_output_pin(ed::PinId(pin), node.output_name(p), node.output_type(p), is_pin_linked(pin));
        }
    } else {
        ImGui::Dummy(ImVec2(pin_icon_size, pin_icon_size));
    }
    ImGui::EndGroup();
    const float outputs_max_x = ImGui::GetItemRectMax().x;

    // Stretch header across the full node width (title alone may be narrower).
    const ImVec2 full_header_max(std::max(header_max.x, std::max(inputs_max_x, outputs_max_x)), header_max.y);

    ImGui::PopID();
    ed::EndNode();

    draw_node_header(node_id, header_min, full_header_max, header_color);
}

}
