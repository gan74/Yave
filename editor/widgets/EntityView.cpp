/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#include "EntityView.h"

#include <editor/context/EditorContext.h>
#include <editor/components/ComponentTraits.h>

#include <yave/ecs/EntityWorld.h>

#include <imgui/yave_imgui.h>

namespace editor {

EntityView::EntityView(ContextPtr cptr) :
		Widget(ICON_FA_OBJECT_GROUP " Entities"),
		ContextLinked(cptr) {
}

void EntityView::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	ecs::EntityWorld& world = context()->world();






	if(ImGui::Button(ICON_FA_PLUS, math::Vec2(24))) {
		ImGui::OpenPopup("Add entity");
	}

	if(ImGui::BeginPopup("Add entity")) {
		if(ImGui::MenuItem("Add entity")) {
			ecs::EntityId id = world.create_entity();
			world.create_component<EditorComponent>(id);
		}
		ImGui::EndPopup();
	}

	ImGui::Spacing();
	if(ImGui::BeginChild("###entities")) {
		for(ecs::EntityId id : world.entities()) {
			EditorComponent* comp = world.component<EditorComponent>(id);
			if(!comp) {
				log_msg("Entity is missing EditorComponent.", Log::Warning);
				continue;
			}

			bool selected = context()->selection().selected_entity() == id;
			if(ImGui::Selectable(fmt(ICON_FA_CUBE " %", comp->name()).data(), selected)) {
				 context()->selection().set_selected(id);
			}
			if(ImGui::IsItemHovered()) {
				_hovered = id;
			}
		}
	}
	ImGui::EndChild();



	if(_hovered.is_valid()) {
		if(ImGui::IsMouseReleased(1)) {
			ImGui::OpenPopup("###contextmenu");
		}

		if(ImGui::BeginPopup("###contextmenu")) {
			if(ImGui::BeginMenu(ICON_FA_PLUS " Add component")) {
				core::Vector<ComponentTraits> traits = all_component_traits();
				for(const ComponentTraits& t : traits) {
					if(t.name.empty()) {
						continue;
					}
					if(ImGui::MenuItem(fmt(ICON_FA_PUZZLE_PIECE " %", t.name).data())) {
						if(!world.create_component(_hovered, t.type)) {
							log_msg("Unable to create component.", Log::Error);
						}
					}
				}
				ImGui::EndMenu();
			}

			ImGui::Separator();
			if(ImGui::Selectable(ICON_FA_TIMES " Delete")) {
				world.remove_entity(_hovered);
				// we don't unselect the ID to make sure that we can handle case where the id is invalid
			}
			ImGui::EndPopup();
		} else {
			_hovered = ecs::EntityId();
		}
	}
}

}
