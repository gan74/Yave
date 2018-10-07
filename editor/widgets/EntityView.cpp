/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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

#include <yave/objects/Light.h>

#include <imgui/imgui.h>

// remove:
#include <yave/material/Material.h>
#include <y/io/File.h>

namespace editor {

static const char* light_type_name(Light::Type type) {
	switch(type) {
		case Light::Directional:
			return "Directional";

		case Light::Point:
			return "Point";

		default:
		break;
	}
	return y_fatal("Unsupported light type.");
}

static AssetPtr<Material> default_material(DevicePtr dptr) {
	static AssetPtr<Material> material;
	if(!material) {
		material = make_asset<Material>(dptr, MaterialData()
				.set_frag_data(SpirVData::deserialized(io::File::open("basic.frag.spv").expected("Unable to load spirv file.")))
				.set_vert_data(SpirVData::deserialized(io::File::open("basic.vert.spv").expected("Unable to load spirv file.")))
			);
	}
	return material;
}

EntityView::EntityView(ContextPtr cptr) : Widget("Entities"), ContextLinked(cptr) {
}

void EntityView::add_light() {
	Scene::Ptr<Light> light = std::make_unique<Light>(Light::Point);
	context()->selection().set_selected(light.get());

	light->radius() = 100.0f;
	light->color() *= 10000.0;
	context()->scene().scene().lights() << std::move(light);
}

void EntityView::paint_ui(CmdBufferRecorder<>&, const FrameToken&) {
	char buffer[256];

	if(ImGui::Button("+", math::Vec2(24))) {
		ImGui::OpenPopup("Add entity");
	}

	if(ImGui::BeginPopup("Add entity")) {
		if(ImGui::MenuItem("Add light")) {
			add_light();
		}
		if(ImGui::MenuItem("Add renderable")) {
			try {
				AssetPtr<StaticMesh> mesh = context()->loader().static_mesh().load("cube.ym");
				auto instance = std::make_unique<StaticMeshInstance>(mesh, default_material(context()->device()));
				instance->transform() = math::Transform<>(math::Vec3(0.0f, 0.0f, 0.0f), math::identity(), math::Vec3(0.1f));
				context()->scene().scene().static_meshes() << std::move(instance);

			} catch(std::exception& e) {
				log_msg(fmt("Error while adding renderable: %", e.what()));
			}
		}
		ImGui::EndPopup();
	}

	if(ImGui::TreeNode("Renderables")) {
		for(const auto& r : context()->scene().scene().renderables()) {
			std::sprintf(buffer, "%s##%p", type_name(*r).data(), static_cast<void*>(r.get()));
			bool selected = r.get() == context()->selection().selected();
			ImGui::Selectable(buffer, &selected);
			if(selected) {
				context()->selection().set_selected(r.get());
			}
		}
		ImGui::TreePop();
	}

	if(ImGui::TreeNode("Meshes")) {
		for(const auto& r : context()->scene().scene().static_meshes()) {
			std::sprintf(buffer, "%s##%p", type_name(*r).data(), static_cast<void*>(r.get()));
			bool selected = r.get() == context()->selection().selected();
			ImGui::Selectable(buffer, &selected);
			if(selected) {
				context()->selection().set_selected(r.get());
			}
		}
		ImGui::TreePop();
	}

	if(ImGui::TreeNode("Lights")) {
		for(const auto& l : context()->scene().scene().lights()) {
			std::sprintf(buffer, "%s##%p", light_type_name(l->type()), static_cast<void*>(l.get()));
			bool selected = l.get() == context()->selection().selected();
			ImGui::Selectable(buffer, &selected);
			if(selected) {
				context()->selection().set_selected(l.get());
			}
		}
		ImGui::TreePop();
	}

}

}
