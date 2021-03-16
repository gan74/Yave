/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include "PropertyPanel.h"
#include "AssetSelector.h"

#include <editor/Selection.h>
#include <editor/EditorWorld.h>
#include <editor/components/EditorComponent.h>
#include <editor/utils/ui.h>

#include <external/imgui/yave_imgui.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace editor {

static bool display_generic_color(reflect::DynObject<> member) {
    if(auto* rgb = member.to<math::Vec3>()) {
        const int color_flags = ImGuiColorEditFlags_NoSidePreview |
                              ImGuiColorEditFlags_NoAlpha |
                              ImGuiColorEditFlags_Float |
                              ImGuiColorEditFlags_InputRGB;

        if(ImGui::ColorButton("Color", math::Vec4(*rgb, 1.0f), color_flags)) {
            ImGui::OpenPopup("Color");
        }

        if(ImGui::BeginPopup("Color")) {
            ImGui::ColorPicker3("##colorpicker", rgb->begin(), color_flags);
            ImGui::EndPopup();
        }
        return true;
    }
    return false;
}

static bool display_positive_float(reflect::DynObject<> member) {
    if(auto* x = member.to<float>()) {
        ImGui::DragFloat("##float", x, 1.0f, 0.0f, std::numeric_limits<float>::max(), "%.2f");
        return true;
    }
    return false;
}

static bool display_generic_string(reflect::DynObject<> member) {
    if(auto* str = member.to<core::String>()) {
        ImGui::TextUnformatted(str->data());
        return true;
    }
    return false;
}

static bool display_bool(reflect::DynObject<> member) {
    if(auto* b = member.to<bool>()) {
        ImGui::Checkbox("##bool", b);
        return true;
    }
    return false;
}


static void display_members(core::Span<reflect::DynObject<>> members) {
    const std::array display_funcs = {
        display_generic_color,
        display_positive_float,
        display_generic_string,
        display_bool,
    };

    for(const auto& member : members) {
        ImGui::PushID(member.name.data());
        ImGui::TextUnformatted(member.name.data());
        for(const auto func : display_funcs) {
            if(func(member)) {
                break;
            }
        }
        ImGui::PopID();
    }
}

PropertyPanel::PropertyPanel() : Widget(ICON_FA_WRENCH " Properties") {
}

void PropertyPanel::on_gui() {
    if(!selection().has_selected_entity()) {
        return;
    }

    const ecs::EntityId id = selection().selected_entity();

    for(const auto& p : world().component_containers()) {
        ecs::ComponentContainerBase* container = p.second.get();

        const auto members = container->reflect_component(id);
        if(!members.is_empty()) {
            const core::String type_name = container->runtime_info().type_name;
            if(ImGui::CollapsingHeader(type_name.data(), ImGuiTreeNodeFlags_DefaultOpen)) {
                display_members(members);
            }
        }
    }
}

}
