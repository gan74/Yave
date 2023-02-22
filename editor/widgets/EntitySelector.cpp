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

EntitySelector::EntitySelector() : Widget("Select an entity") {
    set_modal(true);
}

EntitySelector::EntitySelector(ecs::ComponentTypeIndex filter) :
        Widget(fmt("Select a %", current_world().component_type_name(filter))),
        _filter(filter),
        _has_filter(true) {
    set_modal(true);
}


void EntitySelector::on_gui() {
    const EditorWorld& world = current_world();

    const auto query = _has_filter
        ? world.query<EditorComponent>(world.component_ids(_filter))
        : world.query<EditorComponent>();

    if(query.is_empty()) {
        ImGui::TextDisabled("No viable entities found");
        return;
    }

    y_defer(ImGui::EndChild());
    if(ImGui::BeginChild("##entities")) {
        for(auto&& [id, comp] : query) {
            const auto& [c] = comp;
            if(ImGui::Selectable(fmt_c_str("% %", world.entity_icon(id), c.name().data()))) {
                if(_selected(id)) {
                    close();
                    return;
                }
            }
        }
    }
}

}

