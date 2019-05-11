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

#include <imgui/imgui_yave.h>

namespace editor {

EcsDebug::EcsDebug(ContextPtr cptr) : Widget("ECS debug", ImGuiWindowFlags_AlwaysAutoResize), ContextLinked(cptr) {
}

void EcsDebug::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	ecs::EntityWorld& world = context()->world();

	if(ImGui::Button("Add entity & component")) {
		ecs::EntityId id = world.create_entity();
		world.create_component<EditorComponent>(id, "unanmed component");
		auto inst = std::make_unique<StaticMeshInstance>(device()->device_resources()[DeviceResources::CubeMesh], device()->device_resources()[DeviceResources::EmptyMaterial]);
		world.create_component<RenderableComponent>(id, std::move(inst));
	}

	if(ImGui::Button("Flush world")) {
		world.flush();
	}

	ImGui::Text("%u component types registered", u32(ecs::detail::registered_types_count()));

	ImGui::Separator();
	if(ImGui::TreeNode("Components")) {
		for(const auto& c : world.components<EditorComponent>()) {
			ImGui::Selectable(fmt(ICON_FA_PUZZLE_PIECE " %", c.name()).data());
		}
		ImGui::TreePop();
	}

}

}
