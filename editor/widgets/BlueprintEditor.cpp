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

#include <editor/utils/StringMatcher.h>
#include <editor/utils/ui.h>

#include <yave/utils/color.h>

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

    const float hue = float(hash(u32(type)) & 0xFFFF) / float(0xFFFF);
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

static ed::PinId pin_id(const BlueprintNode& node, usize index, bool is_input) {
    return ed::PinId(uintptr_t(&node) + 1 + (is_input ? 0 : node.input_count()) + index);
}

static PinInfo find_pin(const Blueprint& blueprint, uintptr_t pin) {
    for(const auto& node : blueprint.all_nodes()) {
        const uintptr_t base = uintptr_t(node.get()) + 1;
        const usize in_count = node->input_count();
        if(pin >= base && pin < base + in_count + node->output_count()) {
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

static void draw_pin(ed::PinId id, std::string_view name, BlueprintParamTypeIndex type, bool connected, bool is_input) {
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


BlueprintEditor::BlueprintEditor(BlueprintWorkspace* ws) :
        WorkspaceWidget(ICON_FA_PROJECT_DIAGRAM " Blueprint Editor", ws, ImGuiWindowFlags_NoScrollbar) {

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

void BlueprintEditor::on_gui() {
    ed::SetCurrentEditor(_context);
    y_defer(ed::SetCurrentEditor(nullptr));

    {
        ed::Begin("##blueprint", ImGui::GetContentRegionAvail());

        const Blueprint& blueprint = workspace()->blueprint();

        core::Vector<const void*> linked_outputs;
        for(const auto& node : blueprint.all_nodes()) {
            for(usize i = 0; i != node->input_count(); ++i) {
                if(const void* in = node->input(i)) {
                    linked_outputs << in;
                }
            }
        }
        std::sort(linked_outputs.begin(), linked_outputs.end());

        {
            y_profile_zone("draw nodes");
            for(const auto& node : blueprint.all_nodes()) {
                draw_node(*node, linked_outputs);
            }
        }

        {
            y_profile_zone("draw links");
            for(const auto& dst : blueprint.all_nodes()) {
                for(usize i = 0; i != dst->input_count(); ++i) {
                    if(const void* in = dst->input(i)) {
                        const auto [src_node, src_pin] = blueprint.find_output(in);
                        y_debug_assert(src_node);
                        const ed::PinId end = pin_id(*dst, i, true);
                        ed::Link(ed::LinkId(end.Get()),pin_id(*src_node, src_pin, false), end, pin_type_color(src_node->output_type(src_pin)), 2.0f);
                    }
                }
            }
        }

        process_links();
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
    y_profile();

    ed::SetCurrentEditor(_context);
    y_defer(ed::SetCurrentEditor(nullptr));

    const Blueprint& blueprint = workspace()->blueprint();
    const core::Span nodes = blueprint.all_nodes();

    core::FixedArray<usize> column_sizes(nodes.size());
    core::FixedArray<usize> depths(nodes.size());
    for(usize i = 0; i != nodes.size(); ++i) {
        for(usize k = 0; k != nodes[i]->input_count(); ++k) {
            if(const void* in = nodes[i]->input(k)) {
                const BlueprintNode* src = blueprint.find_output(in).first;
                const auto src_it = std::find_if(nodes.begin(), nodes.end(), [&](const auto& n) { return n.get() == src; });
                depths[i] = std::max(depths[i], depths[usize(src_it - nodes.begin())] + 1);
            }
        }

        const usize col = depths[i];
        const usize row = column_sizes[col]++;
        ed::SetNodePosition(
            ed::NodeId(uintptr_t(nodes[i].get())),
            ImVec2(40.0f + float(col) * layout_cell_size.x, 40.0f + float(row) * layout_cell_size.y)
        );
    }
}

void BlueprintEditor::process_links() {
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
    }
    ed::EndDelete();
}

void BlueprintEditor::draw_context_menu() {
    const math::Vec2 mouse_pos = to_y(ImGui::GetMousePos());

    ed::Suspend();
    y_defer(ed::Resume());

    if(ed::ShowBackgroundContextMenu()) {
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
        const BlueprintParamTypeIndex link_type = !link_pin.node
            ? BlueprintParamTypeIndex::invalid_index
            : link_pin.is_input
                ? link_pin.node->input_type(link_pin.index)
                : link_pin.node->output_type(link_pin.index)
        ;

        const core::Span factories = workspace()->node_factories();
        for(usize i = 0; i != factories.size(); ++i) {
            const auto& factory = factories[i];
            const std::string_view name = factory->name();

            if(!matcher.is_empty() && !matcher.matches(name)) {
                continue;
            }

            usize compatible_index = 0;
            if(link_pin.node) {
                const SharedBlueprintNodeData& data = factory->shared_data();
                const auto& pins = link_pin.is_input ? data.outputs : data.inputs;
                const auto it = std::find_if(pins.begin(), pins.end(), [&](const auto& pin) { return pin.second == link_type; });
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
        } else if(ImGui::BeginMenu("Add node")) {
            show_node_list();
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }
}

void BlueprintEditor::draw_node(const BlueprintNode& node, core::Span<const void*> linked_outputs) {
    const ed::NodeId node_id = ed::NodeId(uintptr_t(&node));
    const std::string_view name = node.name();

    ed::BeginNode(node_id);

    ImGui::TextUnformatted(name.data());
    const float header_bottom = ImGui::GetItemRectMax().y;
    ImGui::Dummy(ImVec2(0.0f, 2.0f));

    ImGui::BeginGroup();
    for(usize i = 0; i != node.input_count(); ++i) {
        draw_pin(pin_id(node, i, true), node.input_name(i), node.input_type(i), node.input(i) != nullptr, true);
    }
    ImGui::EndGroup();

    ImGui::SameLine(0.0f, pin_column_gap);

    ImGui::BeginGroup();
    for(usize i = 0; i != node.output_count(); ++i) {
        const bool linked = std::binary_search(linked_outputs.begin(), linked_outputs.end(), node.output_ptr(i));
        draw_pin(pin_id(node, i, false), node.output_name(i), node.output_type(i), linked, false);
    }
    ImGui::EndGroup();

    ed::EndNode();

    draw_node_header(node_id, header_bottom, node_header_color(name));
}

}
