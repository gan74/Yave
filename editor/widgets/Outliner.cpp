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

#include "Outliner.h"

#include <editor/EditorWorld.h>
#include <editor/components/EditorComponent.h>

#include <editor/utils/assets.h>
#include <editor/utils/ui.h>


namespace editor {


Outliner::Outliner() : Widget(ICON_FA_SITEMAP " Outliner") {
    _tag_buttons.emplace_back(ICON_FA_EYE, ecs::tags::hidden, false);
}

void Outliner::on_gui() {
    EditorWorld& world = current_world();

    if(ImGui::Button(ICON_FA_PLUS)) {
        const ecs::EntityId id = world.create_named_entity("new entity");
        world.set_parent(id, world.selected_entity());
    }

    if(ImGui::BeginTable("##entities", int(1 + _tag_buttons.size()))) {
        y_profile_zone("fill entity table");

        ImGui::TableSetupColumn("##name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##tagbuttons",ImGuiTableColumnFlags_WidthFixed);

        for(const ecs::EntityId id : world.component_ids<EditorComponent>()) {
            if(world.has_parent(id)) {
                continue;
            }

            display_node(world, id);
        }


        ImGui::EndTable();
    }
}

void Outliner::display_node(EditorWorld& world, ecs::EntityId id) {
    const EditorComponent* component = world.component<EditorComponent>(id);
    if(!component) {
        return;
    }


    auto display_tags = [&] {
        ImGui::TableNextColumn();
        for(const auto& [icon, tag, state] : _tag_buttons) {
            const bool tagged = world.has_tag(id, tag);
            if(tagged == state) {
                ImGui::TextUnformatted(icon);
            } else {
                ImGui::TextDisabled("%s", icon);
            }

            if(ImGui::IsItemClicked()) {
                if(tagged) {
                    world.remove_tag(id, tag);
                } else {
                    world.add_tag(id, tag);
                }
            }
        }
    };


    imgui::table_begin_next_row();

    const bool is_selected = world.is_selected(id);
    const bool has_children = world.has_children(id);

    const int flags =
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_OpenOnArrow |
        (is_selected ? ImGuiTreeNodeFlags_Selected : 0) |
        (has_children ? 0 : ImGuiTreeNodeFlags_Leaf)
    ;

    if(ImGui::TreeNodeEx(fmt_c_str("{} {}###{}", world.entity_icon(id), component->name(), id.as_u64()), flags)) {
        if(ImGui::IsItemClicked()) {
            world.toggle_selected(id, !ImGui::GetIO().KeyCtrl);
        }

        display_tags();

        for(const ecs::EntityId child : world.children(id)) {
            display_node(world, child);
        }

        ImGui::TreePop();
    } else {
        display_tags();
    }
}

}

