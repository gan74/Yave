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
#include <editor/properties/ComponentTraits.h>
#include <editor/components/EditorComponent.h>

#include <yave/ecs/EntityWorld.h>
#include <yave/entities/entities.h>

#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/PointLightComponent.h>

#include <imgui/yave_imgui.h>

namespace editor {

static core::String clean_component_name(std::string_view name) {
	if(name.empty()) {
		return name;
	}
	core::String clean;
	clean.push_back(name[0]);

	for(char c : core::Range(name.begin() + 1, name.end())) {
		if(std::isupper(c)) {
			clean.push_back(' ');
		}
		clean.push_back(std::tolower(c));
	}
	return clean;
}

using EditorEmptyEntity = ecs::EntityArchetype<EditorComponent>;
using EditorStaticMesh = StaticMeshArchetype::with<EditorComponent>;
using EditorPointLight = PointLightArchetype::with<EditorComponent>;

static_assert(EditorStaticMesh::component_count == 3);


EntityView::EntityView(ContextPtr cptr) :
		Widget(ICON_FA_CUBES " Entities"),
		ContextLinked(cptr) {
}

void EntityView::paint_view() {
	const ecs::EntityWorld& world = context()->world();
	if(ImGui::BeginChild("###entities")) {
		for(ecs::EntityId id : world.entities()) {
			const EditorComponent* comp = world.component<EditorComponent>(id);
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
}

void EntityView::paint_clustered_view() {
	const ecs::EntityWorld& world = context()->world();
	if(ImGui::BeginChild("###entities")) {
		if(ImGui::TreeNodeEx(ICON_FA_CUBE " Static meshes", ImGuiTreeNodeFlags_DefaultOpen)) {
			for(const auto& [ed, tr, _] : world.view(EditorStaticMesh()).components()) {
				ImGui::TreeNodeEx(fmt(ICON_FA_CUBE " %", ed.name()).data(), ImGuiTreeNodeFlags_Leaf);
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
		if(ImGui::TreeNodeEx(ICON_FA_LIGHTBULB " Point lights", ImGuiTreeNodeFlags_DefaultOpen)) {
			for(const auto& [ed, tr, _] : world.view(EditorPointLight()).components()) {
				ImGui::TreeNodeEx(fmt(ICON_FA_LIGHTBULB " %", ed.name()).data(), ImGuiTreeNodeFlags_Leaf);
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
	}
	ImGui::EndChild();
}

void EntityView::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	ecs::EntityWorld& world = context()->world();

	if(ImGui::Button(ICON_FA_PLUS, math::Vec2(24))) {
		ImGui::OpenPopup("Add entity");
	}
	ImGui::SameLine();
	ImGui::Checkbox("Clustered view", &_clustered_view);


	if(ImGui::BeginPopup("Add entity")) {
		if(ImGui::MenuItem("Add empty entity")) {
			world.create_entity(EditorEmptyEntity());
		}
		ImGui::Separator();
		if(ImGui::MenuItem("Add static mesh")) {
			world.create_entity(EditorStaticMesh());
		}
		if(ImGui::MenuItem("Add point light")) {
			world.create_entity(EditorPointLight());
		}
		ImGui::EndPopup();
	}





	ImGui::Spacing();
	if(_clustered_view) {
		paint_clustered_view();
	} else {
		paint_view();
	}



	if(_hovered.is_valid()) {
		if(ImGui::IsMouseReleased(1)) {
			ImGui::OpenPopup("###contextmenu");
		}

		if(ImGui::BeginPopup("###contextmenu")) {
			if(ImGui::BeginMenu(ICON_FA_PLUS " Add component")) {
				core::Vector<ComponentTraits> traits = all_component_traits();
				for(const ComponentTraits& t : traits) {
					if(t.name.empty() || world.has(_hovered, t.type)) {
						continue;
					}
					if(ImGui::MenuItem(fmt(ICON_FA_PUZZLE_PIECE " %", clean_component_name(t.name)).data())) {
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
