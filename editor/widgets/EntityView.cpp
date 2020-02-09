/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#include <editor/components/EditorComponent.h>

#include <yave/ecs/EntityWorld.h>
#include <yave/entities/entities.h>

#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/SkyComponent.h>

#include <imgui/yave_imgui.h>

namespace editor {

[[maybe_unused]]
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
		clean.push_back(char(std::tolower(c)));
	}
	return clean;
}

class ArchetypeBase {
	public:
		virtual ~ArchetypeBase() {
		}

		const char* name() const {
			return _name;
		}

		const char* icon() const {
			return _icon;
		}

		virtual ecs::EntityId create(ecs::EntityWorld& world) const = 0;
		virtual void for_each(ecs::EntityWorld& world, const core::Function<void(ecs::EntityId)>& func) const = 0;

	protected:
		ArchetypeBase(const char* name, const char* icon) : _name(clean_component_name(name)), _icon(icon) {
		}


		core::String _name = "";
		const char* _icon = ICON_FA_CUBES;
};

template<typename T>
class DynArchtype : public ArchetypeBase {
	public:
		DynArchtype(const char* name, const char* icon) : ArchetypeBase(name, icon) {
		}

		ecs::EntityId create(ecs::EntityWorld& world) const override {
			return world.create_entity(T());
		}

		void for_each(ecs::EntityWorld& world, const core::Function<void(ecs::EntityId)>& func) const override {
			for(auto entity : world.view(T())) {
				ecs::EntityId id = world.id_from_index(entity.index());
				func(id);
			}
		}

};


#define MAKE_ARCHETYPE(name, icon) std::make_unique<DynArchtype<name ## Archetype::with<EditorComponent>>>(#name, icon)

static core::Span<std::unique_ptr<ArchetypeBase>> entity_archtypes() {
	static core::Vector<std::unique_ptr<ArchetypeBase>> archs;
	if(archs.is_empty()) {
		archs << MAKE_ARCHETYPE(StaticMesh, ICON_FA_CUBE);
		archs << MAKE_ARCHETYPE(PointLight, ICON_FA_LIGHTBULB);
		archs << MAKE_ARCHETYPE(SpotLight, ICON_FA_VIDEO);
		archs << MAKE_ARCHETYPE(DirectionalLight, ICON_FA_SUN);
		archs << MAKE_ARCHETYPE(Sky, ICON_FA_CLOUD_SUN);
	}
	return archs;
}

#undef MAKE_ARCHETYPE



EntityView::EntityView(ContextPtr cptr) :
		Widget(ICON_FA_CUBES " Entities"),
		ContextLinked(cptr) {
}

void EntityView::paint_view() {
	const ecs::EntityWorld& world = context()->world();
	
	if(ImGui::BeginChild("###entities", ImVec2(), true)) {
		for(ecs::EntityId id : world.entities()) {
			const EditorComponent* comp = world.component<EditorComponent>(id);
			if(!comp) {
				log_msg("Entity is missing EditorComponent.", Log::Warning);
				continue;
			}

			const bool selected = context()->selection().selected_entity() == id;
			if(ImGui::Selectable(fmt_c_str(ICON_FA_CUBE " %##%", comp->name(), id.index()), selected)) {
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

	auto paint_group = [&](const ArchetypeBase& archetype) {
		const char* icon = archetype.icon();
		ImGui::PushID(archetype.name());

		const auto paint_category_menu = [&] {
			if(ImGui::IsItemClicked(1)) {
				ImGui::OpenPopup("Add entity");
			}
			if(ImGui::BeginPopup("Add entity")) {
				if(ImGui::MenuItem(fmt_c_str("% %", icon, "Add entity"))) {
					archetype.create(context()->world());
				}
				ImGui::EndPopup();
			}
		};

		if(ImGui::TreeNodeEx(fmt_c_str("% %", icon, archetype.name()), ImGuiTreeNodeFlags_DefaultOpen)) {
			paint_category_menu();

			archetype.for_each(context()->world(), [&](ecs::EntityId id) {
				const EditorComponent* editor_comp = world.component<EditorComponent>(id);
				y_debug_assert(editor_comp);

				int flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				if(context()->selection().selected_entity() == id) {
					flags |= ImGuiTreeNodeFlags_Selected;
				}
				ImGui::TreeNodeEx(fmt_c_str("% %##%", icon, editor_comp->name(), id.index()), flags);

				if(ImGui::IsItemClicked()) {
					context()->selection().set_selected(id);
				}
				if(ImGui::IsItemHovered()) {
					_hovered = id;
				}
			});

			ImGui::TreePop();
		} else {
			paint_category_menu();
		}

		ImGui::PopID();
	};

	if(ImGui::BeginChild("###entities", ImVec2(), true)) {
		for(const auto& archetype : entity_archtypes()) {
			paint_group(*archetype);
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
		ecs::EntityId ent;
		if(ImGui::MenuItem("Add empty entity")) {
			ent = world.create_entity();
		}
		ImGui::Separator();

		for(const auto& archetype : entity_archtypes()) {
			if(ImGui::MenuItem(fmt("% Add %", archetype->icon(), archetype->name()).data())) {
				ent = archetype->create(world);
			}
		}
		
		y_debug_assert(!ent.is_valid() || world.has<EditorComponent>(ent));
		y_debug_assert(world.required_component_types().size() > 0);

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

				ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
				ImGui::MenuItem(ICON_FA_EXCLAMATION_TRIANGLE " Adding individual components is not supported");
				ImGui::PopStyleColor();

				/*core::Vector<ComponentTraits> traits = all_component_traits();
				for(const ComponentTraits& t : traits) {
					if(t.name.empty() || world.has(_hovered, t.type)) {
						continue;
					}
					if(ImGui::MenuItem(fmt_ctr(ICON_FA_PUZZLE_PIECE " %", clean_component_name(t.name)))) {
						if(!world.create_component(_hovered, t.type)) {
							log_msg("Unable to create component.", Log::Error);
						}
					}
				}*/
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
