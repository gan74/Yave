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
#include "AssetSelector.h"

#include <editor/utils/StringMatcher.h>
#include <editor/utils/ui.h>

#include <yave/utils/color.h>

#include <y/core/FixedArray.h>
#include <y/utils/format.h>
#include <y/utils/hash.h>

#include <external/imgui-node-editor/imgui_node_editor.h>

#include <algorithm>

namespace editor {

namespace ed = ax::NodeEditor;

struct PinInfo {
    const BlueprintNode* node = nullptr;
    usize index = 0;
    bool is_input = false;
};

static constexpr float pin_icon_size = 16.0f;
static constexpr float pin_column_gap = 20.0f;
static constexpr ImVec4 node_padding = ImVec4(6.0f, 2.0f, 6.0f, 4.0f);
static constexpr ImVec2 layout_cell_size = ImVec2(260.0f, 300.0f);
static constexpr ImColor error_color = ImColor(250, 20, 20, 255);
static constexpr ImColor after_error_color = error_color; // ImColor(255, 100, 70, 255);
static constexpr ImColor arrow_color = ImColor(255, 255, 255, 255);

static ImColor pin_type_color(const BlueprintParamType* type) {
    if(type == blueprint_param_type_index<float>()) {
        return ImColor(147, 226, 74);
    }
    if(type == blueprint_param_type_index<bool>()) {
        return ImColor(220, 48, 48);
    }
    if(!type) {
        return ImColor(255, 255, 255);
    }

    const float hue = float(type->type_hash & 0xFFFF) / float(0xFFFF);
    const math::Vec3 rgb = hsv_to_rgb(hue, 0.65f, 0.9f);
    return ImColor(rgb.x(), rgb.y(), rgb.z());
}

static ImColor node_header_color(std::string_view name) {
    // Green / teal headers; vary slightly per name.
    const u32 h = ct_str_hash(name);
    const int r = 30 + int(h & 0x2F);
    const int g = 120 + int((h >> 8) & 0x4F);
    const int b = 70 + int((h >> 16) & 0x3F);
    return ImColor(r, g, b);
}

static ed::PinId input_pin_id(const BlueprintNode& node, usize index) {
    return ed::PinId(uintptr_t(&node) + 1 + index);
}

static ed::PinId output_pin_id(const BlueprintNode& node, usize index) {
    return ed::PinId(uintptr_t(&node) + 1 + node.input_pins().size() + index);
}

static PinInfo find_pin(const Blueprint& blueprint, uintptr_t pin) {
    for(const auto& node : blueprint.all_nodes()) {
        const uintptr_t base = uintptr_t(node.get()) + 1;
        const usize in_count = node->input_pins().size();
        if(pin >= base && pin < base + in_count + node->output_pins().size()) {
            const usize index = usize(pin - base);
            return index < in_count ? PinInfo{node.get(), index, true} : PinInfo{node.get(), index - in_count, false};
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

static void draw_pin(ed::PinId id, std::string_view name, const BlueprintParamType* type, bool connected, bool is_input) {
    ed::BeginPin(id, is_input ? ed::PinKind::Input : ed::PinKind::Output);
    ed::PinPivotAlignment(ImVec2(is_input ? 0.0f : 1.0f, 0.5f));
    ed::PinPivotSize(ImVec2(0.0f, 0.0f));

    if(is_input) {
        draw_pin_icon(pin_type_color(type), connected);
        ImGui::SameLine();
    }
    ImGui::TextUnformatted(name.data(), name.data() + name.size());
    if(!is_input) {
        ImGui::SameLine();
        draw_pin_icon(pin_type_color(type), connected);
    }

    ed::EndPin();
}

static void draw_node_header(ed::NodeId node_id, float header_bottom, ImColor header_color) {
    if(!ImGui::IsItemVisible()) {
        return;
    }

    ImDrawList* draw_list = ed::GetNodeBackgroundDrawList(node_id);
    const float alpha = ImGui::GetStyle().Alpha;
    const float half_border = ed::GetStyle().NodeBorderWidth * 0.5f;
    const ImU32 color = IM_COL32(0, 0, 0, int(255.0f * alpha)) | (ImU32(header_color) & IM_COL32(255, 255, 255, 0));

    const ImVec2 min = ImGui::GetItemRectMin() + ImVec2(half_border, half_border);
    const ImVec2 max = ImVec2(ImGui::GetItemRectMax().x - half_border, header_bottom);

    draw_list->AddRectFilled(min, max, color, ed::GetStyle().NodeRounding, ImDrawFlags_RoundCornersTop);
    draw_list->AddLine(
        ImVec2(min.x, max.y - 0.5f),
        ImVec2(max.x, max.y - 0.5f),
        IM_COL32(255, 255, 255, int(96.0f * alpha / 3.0f)),
        1.0f
    );
}

static void draw_error_label(const BlueprintNode& node, const core::String& error) {
    const ed::NodeId node_id = uintptr_t(&node);
    const ImVec2 pos = ed::GetNodePosition(node_id);
    const ImVec2 size = ed::GetNodeSize(node_id);

    const ImVec2 text_size = ImGui::CalcTextSize(error.data());
    const ImVec2 padding(6.0f, 4.0f);
    const ImVec2 box_size = text_size + padding * 2.0f;
    const ImVec2 box_min(pos.x + (size.x - box_size.x) * 0.5f, pos.y - box_size.y - 4.0f);
    const ImVec2 box_max = box_min + box_size;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(box_min, box_max, ImColor(40, 10, 10, 230), 3.0f);
    draw_list->AddRect(box_min, box_max, error_color, 3.0f);
    draw_list->AddText(box_min + padding, error_color, error.data());
}


static void draw_node(const BlueprintNode& node, const BlueprintParamType* generic_type, core::Span<uintptr_t> linked_pins) {
    const ed::NodeId node_id = ed::NodeId(uintptr_t(&node));
    const std::string_view name = node.name();

    const auto is_linked = [&](ed::PinId id) {
        return std::binary_search(linked_pins.begin(), linked_pins.end(), uintptr_t(id.Get()));
    };

    ed::BeginNode(node_id);

    ImGui::TextUnformatted(name.data());
    const float header_bottom = ImGui::GetItemRectMax().y;
    ImGui::Dummy(ImVec2(0.0f, 2.0f));

    const core::Span<BlueprintPin> inputs = node.input_pins();
    const core::Span<BlueprintPin> outputs = node.output_pins();

    ImGui::BeginGroup();
    for(usize i = 0; i != inputs.size(); ++i) {
        draw_pin(input_pin_id(node, i), inputs[i].name, inputs[i].is_generic ? generic_type : inputs[i].type, is_linked(input_pin_id(node, i)), true);
    }
    ImGui::EndGroup();

    ImGui::SameLine(0.0f, pin_column_gap);

    ImGui::BeginGroup();
    for(usize i = 0; i != outputs.size(); ++i) {
        draw_pin(output_pin_id(node, i), outputs[i].name, outputs[i].is_generic ? generic_type : outputs[i].type, is_linked(output_pin_id(node, i)), false);
    }
    ImGui::EndGroup();

    ed::EndNode();

    draw_node_header(node_id, header_bottom, node_header_color(name));
}





BlueprintEditor::BlueprintEditor(BlueprintWorkspace* ws) :
        WorkspaceWidget(ICON_FA_PROJECT_DIAGRAM " Blueprint Editor", ws, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar) {

    ed::Config config;
    config.SettingsFile = nullptr;
    config.SaveSettings = [](const char*, size_t, ed::SaveReasonFlags, void*) { return true; };
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

    reset_node_layout();
}

BlueprintEditor::~BlueprintEditor() {
    ed::DestroyEditor(_context);
}

void BlueprintEditor::center_on_node(const BlueprintNode* node) {
    ed::SetCurrentEditor(_context);
    y_defer(ed::SetCurrentEditor(nullptr));

    ed::SelectNode(ed::NodeId(uintptr_t(node)));
    ed::NavigateToSelection();
}

void BlueprintEditor::on_gui() {
    const Blueprint& blueprint = workspace()->blueprint();
    const core::Span nodes = blueprint.all_nodes();

    const auto& result = workspace()->result();
    const usize error_node_index = result.is_error() ? result.error().node_index : usize(-1);
    const BlueprintNode* error_node = error_node_index < nodes.size() ? nodes[error_node_index].get() : nullptr;

    if(ImGui::BeginMenuBar()) {
        ImGui::Checkbox("Show execution order", &_show_execution_order);
        ImGui::Separator();
        ImGui::TextUnformatted(fmt_c_str("{} nodes", nodes.size()));

        if(error_node) {
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Text, imgui::error_text_color);
            if(ImGui::Selectable(fmt_c_str("Node \"{}\": {}", error_node->name(), result.error().error))) {
                center_on_node(error_node);
            }
            ImGui::PopStyleColor();
        }

        ImGui::EndMenuBar();
    }

    ed::SetCurrentEditor(_context);
    y_defer(ed::SetCurrentEditor(nullptr));

    {
        ed::Begin("##blueprint", ImGui::GetContentRegionAvail());

        core::FixedArray<const BlueprintParamType*> generic_types(nodes.size());
        core::Vector<uintptr_t> linked_pins;
        {
            for(usize i = 0; i != nodes.size(); ++i) {
                generic_types[i] = blueprint.generic_type(i);
            }

            for(const BlueprintLink& link : blueprint.links()) {
                linked_pins << output_pin_id(*nodes[link.src_node], link.src_pin).Get();
                linked_pins << input_pin_id(*nodes[link.dst_node], link.dst_pin).Get();
            }
            std::sort(linked_pins.begin(), linked_pins.end());
            linked_pins.shrink_to(usize(std::unique(linked_pins.begin(), linked_pins.end()) - linked_pins.begin()));
        }

        {
            y_profile_zone("draw nodes");
            for(usize i = 0; i != nodes.size(); ++i) {
                if(i >= error_node_index) {
                    const bool is_error = i == error_node_index;
                    ed::PushStyleColor(ed::StyleColor_NodeBorder, is_error ? error_color : after_error_color);
                    ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, is_error ? 6.0f : 1.5f);
                }

                draw_node(*nodes[i], generic_types[i], linked_pins);

                if(i >= error_node_index) {
                    ed::PopStyleVar();
                    ed::PopStyleColor();
                }
            }
        }

        {
            y_profile_zone("draw links");
            for(const BlueprintLink& link : blueprint.links()) {
                const BlueprintNode& src = *nodes[link.src_node];
                const BlueprintPin& src_pin = src.output_pins()[link.src_pin];
                const ed::PinId start = output_pin_id(src, link.src_pin);
                const ed::PinId end = input_pin_id(*nodes[link.dst_node], link.dst_pin);
                ed::Link(ed::LinkId(end.Get()), start, end, pin_type_color(src_pin.is_generic ? generic_types[link.src_node] : src_pin.type), 2.0f);
            }
        }

        if(error_node) {
            draw_error_label(*error_node, result.error().error);
        }

        if(_show_execution_order) {
            draw_execution_order();
        }

        process_actions();
        draw_context_menu();

        ed::End();
    }

    if(ed::HasSelectionChanged()) {
        ed::NodeId id;
        if(ed::GetSelectedNodes(&id, 1) == 1) {
            workspace()->set_selected_node(reinterpret_cast<BlueprintNode*>(id.Get()));
        } else {
            workspace()->set_selected_node(nullptr);
        }
    }
}

void BlueprintEditor::reset_node_layout() {
    layout_nodes(0, math::Vec2(40.0f));
}

void BlueprintEditor::layout_nodes(usize first_node, math::Vec2 origin) {
    y_profile();

    ed::SetCurrentEditor(_context);
    y_defer(ed::SetCurrentEditor(nullptr));

    const Blueprint& blueprint = workspace()->blueprint();
    const core::Span nodes = blueprint.all_nodes().take(first_node);

    core::FixedArray<usize> column_sizes(nodes.size());
    core::FixedArray<usize> depths(nodes.size());
    for(usize i = 0; i != nodes.size(); ++i) {
        for(const BlueprintLink& link : blueprint.links()) {
            if(link.dst_node == first_node + i && link.src_node >= first_node) {
                depths[i] = std::max(depths[i], depths[link.src_node - first_node] + 1);
            }
        }

        const usize col = depths[i];
        const usize row = column_sizes[col]++;
        ed::SetNodePosition(
            ed::NodeId(uintptr_t(nodes[i].get())),
            ImVec2(origin.x() + float(col) * layout_cell_size.x, origin.y() + float(row) * layout_cell_size.y)
        );
    }
}

void BlueprintEditor::process_actions() {
    y_profile();

    Blueprint& blueprint = workspace()->blueprint();

    if(ed::BeginCreate()) {
        ed::PinId start_id;
        ed::PinId end_id;
        if(ed::QueryNewLink(&start_id, &end_id) && start_id && end_id) {
            PinInfo start = find_pin(blueprint, start_id.Get());
            PinInfo end = find_pin(blueprint, end_id.Get());

            if(start.node && start.is_input) {
                std::swap(start, end);
            }

            const bool valid =
                start.node && end.node &&
                !start.is_input && end.is_input &&
                blueprint.is_link_valid(start.node, start.index, end.node, end.index)
            ;

            if(valid) {
                if(ed::AcceptNewItem()) {
                    blueprint.add_link(start.node, start.index, end.node, end.index);
                }
            } else {
                ed::RejectNewItem();
            }
        }

        ed::PinId pin_id;
        if(ed::QueryNewNode(&pin_id) && ed::AcceptNewItem()) {
            _new_node.link_pin = pin_id.Get();
            _open_node_menu = true;
        }
    }
    ed::EndCreate();

    if(ed::BeginDelete()) {
        ed::LinkId link_id;
        while(ed::QueryDeletedLink(&link_id)) {
            if(ed::AcceptDeletedItem()) {
                const PinInfo end = find_pin(blueprint, link_id.Get());
                if(end.node && end.is_input) {
                    blueprint.remove_link(end.node, end.index);
                }
            }
        }

        ed::NodeId node_id;
        while(ed::QueryDeletedNode(&node_id)) {
            if(ed::AcceptDeletedItem()) {
                const BlueprintNode* node = reinterpret_cast<const BlueprintNode*>(node_id.Get());
                if(workspace()->selected_node() == node) {
                    workspace()->set_selected_node(nullptr);
                }
                blueprint.remove_node(node);
            }
        }
    }
    ed::EndDelete();
}

void BlueprintEditor::draw_context_menu() {
    const math::Vec2 mouse_pos = to_y(ImGui::GetMousePos());

    ed::Suspend();
    y_defer(ed::Resume());

    // The node editor doesn't know about other windows on top of it
    if(ed::ShowBackgroundContextMenu() && ImGui::IsWindowHovered()) {
        _open_node_menu = true;
        _new_node.link_pin = 0;
    }

    if(_open_node_menu) {
        _open_node_menu = false;
        _new_node.pos = mouse_pos;
        _node_filter.make_empty();
        ImGui::OpenPopup("##contextmenu");
    }

    const auto show_node_list = [&] {
        if(ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }

        imgui::text_input("##filter", _node_filter, ImGuiInputTextFlags_AutoSelectAll, ICON_FA_SEARCH " Search");
        const StringMatcher matcher(_node_filter);

        Blueprint& blueprint = workspace()->blueprint();
        const PinInfo link_pin = find_pin(blueprint, _new_node.link_pin);
        const BlueprintParamType* link_type = nullptr;
        if(link_pin.node) {
            const BlueprintPin& pin = link_pin.is_input ? link_pin.node->input_pins()[link_pin.index] : link_pin.node->output_pins()[link_pin.index];
            link_type = pin.is_generic ? blueprint.generic_type(blueprint.find_node_index(link_pin.node)) : pin.type;
        }

        const core::Span factories = workspace()->node_factories();
        for(usize i = 0; i != factories.size(); ++i) {
            const auto& factory = factories[i];
            const std::string_view name = factory->name();

            if(!matcher.is_empty() && !matcher.matches(name)) {
                continue;
            }

            usize compatible_index = 0;
            if(link_pin.node) {
                const std::unique_ptr<BlueprintNode> prototype = factory->create_node();
                const core::Span<BlueprintPin> pins = link_pin.is_input ? prototype->output_pins() : prototype->input_pins();
                const auto it = std::find_if(pins.begin(), pins.end(), [&](const BlueprintPin& pin) { return pin.is_generic || !link_type || pin.type == link_type; });
                if(it != pins.end()) {
                    compatible_index = usize(it - pins.begin());
                } else {
                    continue;
                }
            }

            if(ImGui::MenuItem(fmt_c_str("{}##{}", name, i))) {
                const BlueprintNode* node = blueprint.add_node(factory->create_node());
                ed::SetNodePosition(ed::NodeId(uintptr_t(node)), to_im(_new_node.pos));
                if(link_pin.node) {
                    if(link_pin.is_input) {
                        blueprint.add_link(node, compatible_index, link_pin.node, link_pin.index);
                    } else {
                        blueprint.add_link(link_pin.node, link_pin.index, node, compatible_index);
                    }
                }
            }
        }
    };

    if(ImGui::BeginPopup("##contextmenu")) {
        if(_new_node.link_pin) {
            show_node_list();
        } else {
            if(ImGui::BeginMenu("Add node")) {
                show_node_list();
                ImGui::EndMenu();
            }

            if(ImGui::MenuItem("Add blueprint")) {
                const math::Vec2 pos = _new_node.pos;
                add_top_level_widget<AssetSelector>(AssetType::Blueprint, "Add blueprint")->set_selected_callback(
                    [this, pos](AssetId id) {
                        const usize first_node = workspace()->blueprint().all_nodes().size();
                        if(!workspace()->add_blueprint(id)) {
                            return false;
                        }
                        layout_nodes(first_node, pos);
                        return true;
                    }
                );
            }
        }
        ImGui::EndPopup();
    }
}

void BlueprintEditor::draw_execution_order() {
    const core::Span nodes = workspace()->blueprint().all_nodes();
    if(nodes.is_empty()) {
        return;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const float thickness = 2.5f;
    const float arrow_size = 14.0f;

    for(usize i = 0; i != nodes.size(); ++i) {
        const ed::NodeId node_id(uintptr_t(nodes[i].get()));
        const ImVec2 pos = ed::GetNodePosition(node_id);
        const ImVec2 size = ed::GetNodeSize(node_id);
        const char* label = fmt_c_str("{}", i);
        const ImVec2 text_size = ImGui::CalcTextSize(label);
        draw_list->AddText(ImVec2(pos.x + (size.x - text_size.x) * 0.5f, pos.y - text_size.y), arrow_color, label);
    }

    for(usize i = 0; i + 1 < nodes.size(); ++i) {
        const ed::NodeId from_id(uintptr_t(nodes[i].get()));
        const ed::NodeId to_id(uintptr_t(nodes[i + 1].get()));

        const ImVec2 from_center = ed::GetNodePosition(from_id) + ed::GetNodeSize(from_id) * 0.5f;
        const ImVec2 to_center = ed::GetNodePosition(to_id) + ed::GetNodeSize(to_id) * 0.5f;

        auto border_point = [](ed::NodeId id, ImVec2 toward) {
            const ImVec2 pos = ed::GetNodePosition(id);
            const ImVec2 half = ed::GetNodeSize(id) * 0.5f;
            const ImVec2 center = pos + half;
            const ImVec2 delta = toward - center;

            float t = std::numeric_limits<float>::max();
            if(delta.x != 0.0f) {
                t = std::min(t, half.x / std::abs(delta.x));
            }
            if(delta.y != 0.0f) {
                t = std::min(t, half.y / std::abs(delta.y));
            }
            return center + delta * t;
        };

        const ImVec2 from = border_point(from_id, to_center);
        const ImVec2 to = border_point(to_id, from_center);

        const ImVec2 delta = to - from;
        const float len = to_y(delta).length();
        if(len > 1.0f) {
            const ImVec2 dir = delta / len;
            const ImVec2 perp(-dir.y, dir.x);
            const ImVec2 base = to - dir * arrow_size;

            draw_list->AddLine(from, base, arrow_color, thickness);
            draw_list->AddTriangleFilled(
                to,
                base + perp * (arrow_size * 0.45f),
                base - perp * (arrow_size * 0.45f),
                arrow_color
            );
        }
    }
}

}
