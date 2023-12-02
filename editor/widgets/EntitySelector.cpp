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

#include "EntitySelector.h"

#include <editor/EditorWorld.h>
#include <editor/components/EditorComponent.h>

#include <editor/utils/ui.h>

namespace editor {

EntitySelector::EntitySelector(ecs::ComponentTypeIndex filter) :
        Widget(filter == ecs::ComponentTypeIndex::invalid_index
            ? "Select an entity"
            : fmt("Select a {}", current_world().component_type_name(filter))),
        _filter(filter) {
    set_modal(true);
}


void EntitySelector::on_gui() {
    const EditorWorld& world = current_world();

    const bool has_filter = (_filter != ecs::ComponentTypeIndex::invalid_index);

    if(has_filter) {
        ImGui::Checkbox("Show all entities", &_show_all);
    }

    const auto query = (has_filter && !_show_all)
        ? world.query<EditorComponent>(world.component_ids(_filter).ids())
        : world.query<EditorComponent>();

    if(ImGui::BeginChild("##entities")) {
        if(query.is_empty()) {
            ImGui::TextDisabled("No viable entities found");
        }

        for(auto&& [id, comp] : query) {
            const auto& [c] = comp;
            imgui::text_icon(world.entity_icon(id));
            ImGui::SameLine();
            if(ImGui::Selectable(c.name().data())) {
                if(_selected(id)) {
                    close();
                    break;
                }
            }
        }
    }
    ImGui::EndChild();
}

}

