/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include <editor/context/scenes.h>
#include <editor/context/EditorContext.h>

#include <yave/material/Material.h>
#include <yave/objects/StaticMeshInstance.h>

#include <y/io/File.h>

#include <imgui/imgui.h>

namespace editor {

static AssetPtr<Material> create_material(ContextPtr cptr) {
	return make_asset<Material>(cptr->device(), MaterialData()
			.set_frag_data(SpirVData::deserialized(io::File::open("basic.frag.spv").expected("Unable to load spirv file.")))
			.set_vert_data(SpirVData::deserialized(io::File::open("basic.vert.spv").expected("Unable to load spirv file.")))
		);
}

static AssetPtr<StaticMesh> create_mesh(ContextPtr cptr) {
	return cptr->loader().static_mesh().import("cube.ym", "../meshes/cube.obj.ym");
}

SceneDebug::SceneDebug(ContextPtr cptr) : Widget("Scene debug", ImGuiWindowFlags_AlwaysAutoResize), ContextLinked(cptr) {
}

void SceneDebug::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	if(ImGui::Button("Spawn cubes")) {
		auto cam_pos = context()->scene().scene_view().camera().position();

		try {
			auto material = create_material(context());
			auto mesh = create_mesh(context());
			float mul = mesh->radius() * 5.0f;

			i32 size = 10;
			for(i32 x = -size; x != size; ++x) {
				for(i32 y = -size; y != size; ++y) {
					for(i32 z = -size; z != size; ++z) {
						auto inst = std::make_unique<StaticMeshInstance>(mesh, material);
						inst->position() = cam_pos + math::Vec3(x, y, z) * mul;
						context()->scene().scene().static_meshes().emplace_back(std::move(inst));
					}
				}
			}
		} catch(std::exception& e) {
			log_msg(fmt("Unable to add cubes: %", e.what()), Log::Error);
		}

	}

	if(ImGui::Button("Add skinned mesh")) {
		add_skinned_mesh(&context()->scene().scene(), context()->loader().texture(), context()->loader().static_mesh());
	}

	ImGui::Text("Static meshes: %u", unsigned(context()->scene().scene().static_meshes().size()));
	ImGui::Text("Renderables:   %u", unsigned(context()->scene().scene().renderables().size()));
	ImGui::Text("Lights:        %u", unsigned(context()->scene().scene().lights().size()));
}

}
