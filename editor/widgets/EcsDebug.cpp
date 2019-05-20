/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include "EcsDebug.h"

#include <editor/context/EditorContext.h>
#include <editor/components/EditorComponent.h>
#include <yave/components/RenderableComponent.h>

#include <y/io2/File.h>
#include <y/math/random.h>

#include <imgui/yave_imgui.h>

#include <map>

namespace editor {

EcsDebug::EcsDebug(ContextPtr cptr) : Widget("ECS debug", ImGuiWindowFlags_AlwaysAutoResize), ContextLinked(cptr) {
}

void EcsDebug::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	struct Foo { int i = 0; }; struct Bar { int i = 0; };
	struct Quux { int i = 0; }; struct Cmp { int i = 0; }; struct Bli { int i = 0; };
	ecs::EntityWorld& world = context()->world();


	if(ImGui::Button("Add entity")) {
		_id = world.create_entity();
		y_debug_assert(_id.is_valid());
		world.create_component<EditorComponent>(_id, fmt("Entity #%", _id.index()));
	}

	if(_id.is_valid()) {
		ImGui::SameLine();
		if(ImGui::Button("Add component")) {
			math::FastRandom rng(core::Chrono::program().to_nanos());

			world.create_or_find_component<Foo>(_id);
			if(rng() % 2 == 0) {
				world.create_or_find_component<Bar>(_id);
			}
			if(rng() % 3 == 0) {
				world.create_or_find_component<Quux>(_id);
			}
			if(rng() % 4 == 0) {
				world.create_or_find_component<Cmp>(_id);
			}
			if(rng() % 5 == 0) {
				world.create_or_find_component<Bli>(_id);
			}
		}
	}


	if(ImGui::Button("Flush world")) {
		world.flush();
	}

	if(ImGui::Button(ICON_FA_SAVE " Save")) {
		if(auto file = io2::File::create("world.yw")) {
			WritableAssetArchive ar(file.unwrap());
			if(!world.serialize(ar)) {
				log_msg("Unable to serialize world");
			}
		} else {
			log_msg("Unable to open file");
		}
	}

	ImGui::SameLine();
	if(ImGui::Button(ICON_FA_FOLDER " Load")) {

		if(auto file = io2::File::open("world.yw")) {
			ecs::EntityWorld w;
			ReadableAssetArchive ar(file.unwrap(), context()->loader());
			if(!w.deserialize(ar)) {
				log_msg("Unable to load world.");
			}
			world = std::move(w);
		} else {
			log_msg("Unable to open file.");
		}
	}

	ImGui::SameLine();
	if(ImGui::Button(ICON_FA_REDO " Reset")) {
		world = ecs::EntityWorld();
	}

	ImGui::Spacing();
	ImGui::Text("%u entities", u32(world.entities().size()));

	ImGui::Spacing();
	ImGui::Text("%u component types registered", u32(ecs::detail::registered_types_count()));
	ImGui::Text("%u component types intanced", u32(world.component_containers().size()));

	auto clean_name = [](std::string_view str) {
			auto last = str.find_last_of("::");
			return last == std::string_view::npos ? str : str.substr(last + 1);
		};

	ImGui::Spacing();
	if(ImGui::TreeNode("Component types")) {
		for(const auto& p : world.component_containers()) {
			ecs::ComponentTypeIndex type = p.first;
			ImGui::Selectable(fmt(ICON_FA_LIST_ALT " %", clean_name(world.type_name(type))).data());
		}

		ImGui::TreePop();
	}


	ImGui::Spacing();
	if(ImGui::TreeNode("Components")) {
		std::map<typename ecs::EntityIndex, core::Vector<core::String>> entities;
		for(auto& type : world.component_containers()) {
			core::String name = clean_name(world.type_name(type.first));
			for(auto ids : world.indexes(type.first)) {
				entities[ids] << name;
			}
		}
		for(ecs::EntityId id : world.entities()) {
			EditorComponent* comp = world.component<EditorComponent>(id);
			if(!comp) {
				continue;
			}

			if(ImGui::TreeNode(fmt(ICON_FA_CUBE " %###%", comp->name(), id.index()).data())) {
				usize index = 0;
				for(const core::String& n : entities[id.index()]) {
					ImGui::Selectable(fmt(ICON_FA_PUZZLE_PIECE " %###%", n, index++).data());
				}
				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}
}

}
