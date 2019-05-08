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

#include "SceneDebug.h"

#include <editor/context/EditorContext.h>

#include <yave/material/Material.h>
#include <yave/objects/StaticMeshInstance.h>

#include <y/io/File.h>

#include <imgui/imgui_yave.h>

namespace editor {


SceneDebug::SceneDebug(ContextPtr cptr) : Widget("Scene debug", ImGuiWindowFlags_AlwaysAutoResize), ContextLinked(cptr) {
}

void SceneDebug::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	if(ImGui::Button("Spawn cubes")) {
		auto cam_pos = context()->scene().scene_view().camera().position();

		AssetPtr<StaticMesh> mesh = device()->device_resources()[DeviceResources::CubeMesh];
		float mul = mesh->radius() * 5.0f;

		i32 size = 10;
		for(i32 x = -size; x != size; ++x) {
			for(i32 y = -size; y != size; ++y) {
				for(i32 z = -size; z != size; ++z) {
					auto inst = std::make_unique<StaticMeshInstance>(mesh, device()->device_resources()[DeviceResources::EmptyMaterial]);
					inst->position() = cam_pos + math::Vec3(x, y, z) * mul;
					context()->scene().scene().static_meshes().emplace_back(std::move(inst));
				}
			}
		}
	}

	ImGui::Text("Static meshes: %u", unsigned(context()->scene().scene().static_meshes().size()));
	ImGui::Text("Renderables:   %u", unsigned(context()->scene().scene().renderables().size()));
	ImGui::Text("Lights:        %u", unsigned(context()->scene().scene().lights().size()));
}

}
