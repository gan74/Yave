/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include "ComponentPanel.h"
#include "ComponentPanelWidgets.h"

#include <editor/UndoStack.h>
#include <editor/EditorWorld.h>
#include <editor/utils/ui.h>
#include <editor/components/EditorComponent.h>

#include <yave/ecs/EntityPrefab.h>

#include <y/utils/log.h>
#include <y/utils/format.h>



namespace editor {


ComponentPanel::ComponentPanel() : Widget(ICON_FA_WRENCH " Components") {
    for(auto* link = ComponentPanelWidgetBase::_first_link; link; link = link->next) {
        _widgets.emplace_back(link->create());
        log_msg(fmt("Registered component widget for %", _widgets.last()->runtime_info().type_name), Log::Debug);
    }
}

void ComponentPanel::on_gui() {
    const ecs::EntityId id = current_world().selected_entity();

    if(!id.is_valid()) {
        if(const usize selected_count = current_world().selected_entity_count(); selected_count > 1) {
            ImGui::Text("%u selected entities", u32(selected_count));
        } else {
            ImGui::TextUnformatted("No entity selected");
        }
        return;
    }

    EditorWorld& world = current_world();

    if(const EditorComponent* component = world.component<EditorComponent>(id)) {
        if(component->is_collection()) {
            return;
        }
    }

    {
        core::FlatHashMap<ecs::ComponentTypeIndex, bool> has_widget;
        for(auto& widget : _widgets) {
            const ecs::ComponentRuntimeInfo rt_info = widget->runtime_info();
            has_widget[rt_info.type_id] = true;

            if(!world.has(id, rt_info.type_id)) {
                continue;
            }

            const char* component_name = fmt_c_str(ICON_FA_PUZZLE_PIECE " %", widget->component_name());
            if(ImGui::CollapsingHeader(component_name)) {
                ImGui::Indent();
                widget->process_entity(current_world(), id);
                ImGui::Unindent();
            }
        }

        for(const auto& [name, info] : EditorWorld::component_types()) {
            if(!has_widget.contains(info.type_id)) {
                if(!world.has(id, info.type_id)) {
                    continue;
                }

                const char* type_name = fmt_c_str(ICON_FA_PUZZLE_PIECE " %", info.clean_component_name());
                if(ImGui::CollapsingHeader(type_name)) {
                    ImGui::TextDisabled("Empty");
                }
            }
        }
    }

    if(ImGui::CollapsingHeader(ICON_FA_TAGS " Tags")) {
        ImGui::Indent();
        for(const core::String& tag : world.tags()) {
            if(world.has_tag(id, tag)) {
                ImGui::TextUnformatted(tag.data());
            }
        }
        if(ImGui::Button(ICON_FA_PLUS " Add tag")) {
            auto all = world.tags();
            world.add_tag(id, fmt("tag #%", std::distance(all.begin(), all.end()) + 1));
        }
        ImGui::Unindent();
    }


    const bool editing = ImGui::IsWindowFocused() && ImGui::IsAnyItemActive();
    _editing = editing;


    ImGui::Separator();

    if(ImGui::Button(ICON_FA_PLUS " Add component")) {
        ImGui::OpenPopup("##addcomponentmenu");
    }

    if(ImGui::BeginPopup("##addcomponentmenu")) {
        for(const auto& [name, info] : EditorWorld::component_types()) {
            const bool enabled = !name.is_empty() && !world.has(id, info.type_id) && info.add_component;
            if(ImGui::MenuItem(fmt_c_str(ICON_FA_PUZZLE_PIECE " %", name), nullptr, nullptr, enabled) && enabled) {
                info.add_component(world, id);
            }
        }
        ImGui::EndPopup();
    }
}

}

