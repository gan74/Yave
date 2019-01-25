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

#include "MeshImporter.h"

#include <editor/context/EditorContext.h>
#include <y/io/Buffer.h>

#include <editor/import/transforms.h>

#include <imgui/imgui.h>

namespace editor {

MeshImporter::MeshImporter(ContextPtr ctx, const core::String& import_path) :
		Widget("Mesh importer"),
		ContextLinked(ctx),
		_import_path(import_path) {

	_browser.set_has_parent(true);
	_browser.set_extension_filter(import::supported_scene_extensions());
	_browser.set_canceled_callback([this] { close(); return true; });
	_browser.set_selected_callback([this](const auto& filename) {
			//import_async(filename);
			_filename = filename;
			_state = State::Settings;
			set_flags(ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
			return true;
		});
}

void MeshImporter::paint_ui(CmdBufferRecorder& recorder, const FrameToken& token)  {
	using import::SceneImportFlags;
	if(_state == State::Browsing) {
		_browser.paint(recorder, token);
	} else if(_state == State::Settings) {
		bool import_meshes = (_flags & SceneImportFlags::ImportMeshes) != SceneImportFlags::None;
		bool import_anims = (_flags & SceneImportFlags::ImportAnims) != SceneImportFlags::None;
		bool import_images = (_flags & SceneImportFlags::ImportImages) != SceneImportFlags::None;

		ImGui::Checkbox("Import meshes", &import_meshes);
		ImGui::Checkbox("Import animations", &import_anims);
		ImGui::Checkbox("Import images", &import_images);

		_flags = (import_meshes ? SceneImportFlags::ImportMeshes : SceneImportFlags::None) |
				 (import_anims ? SceneImportFlags::ImportAnims : SceneImportFlags::None) |
				 (import_images ? SceneImportFlags::ImportImages : SceneImportFlags::None)
			;


		ImGui::Separator();

		const char* axes[] = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};
		if(ImGui::BeginCombo("Forward", axes[_forward_axis])) {
			for(usize i = 0; i != 6; ++i) {
				if(ImGui::Selectable(axes[i])) {
					_forward_axis = i;
					if(_forward_axis / 2 == _up_axis / 2) {
						_up_axis = (_up_axis + 2) % 6;
					}
				}
			}
			ImGui::EndCombo();
		}
		if(ImGui::BeginCombo("Up", axes[_up_axis])) {
			for(usize i = 0; i != 6; ++i) {
				if(ImGui::Selectable(axes[i])) {
					_up_axis = i;
					if(_forward_axis / 2 == _up_axis / 2) {
						_forward_axis = (_forward_axis + 2) % 6;
					}
				}
			}
			ImGui::EndCombo();
		}


		if(ImGui::Button("Ok")) {
			_state = State::Importing;
			_import_future = std::async(std::launch::async, [=] {
				return import::import_scene(_filename, _flags);
			});
		}
		ImGui::SameLine();
		if(ImGui::Button("Cancel")) {
			close();
		}
	} else {
		if(done_loading()) {
			try {
				import(std::move(_import_future.get()));
			} catch(std::exception& e) {
				context()->ui().ok("Unable to import", fmt("Unable to import scene: %" , e.what()).data());
				_browser.show();
			}
			close();
		} else {
			ImGui::Text("Loading...");
		}
	}
}

bool MeshImporter::done_loading() const {
	return _import_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

void MeshImporter::import(import::SceneData scene) {
	y_profile();
	auto import_assets = [this](const auto& assets) {
		for(const auto& a : assets) {
			try {
				core::String name = context()->asset_store().filesystem()->join(_import_path, a.name());
				log_msg(fmt("Saving asset as \"%\"", name));
				io::Buffer data;
				serde::serialize(data, a.obj());
				context()->asset_store().import(data, name);
			} catch(std::exception& e) {
				log_msg(fmt("Unable save \"%\": %", a.name(), e.what()), Log::Error);
			}
		}
	};


	if(_forward_axis != 0 || _up_axis != 4) {
		math::Vec3 axes[] = {{1.0f, 0.0f, 0.0f}, {-1.0f,  0.0f,  0.0f},
							 {0.0f, 1.0f, 0.0f}, { 0.0f, -1.0f,  0.0f},
							 {0.0f, 0.0f, 1.0f}, { 0.0f,  0.0f, -1.0f}};
		math::Transform<> transform;
		transform.set_basis(axes[_forward_axis], axes[_up_axis]);

		for(auto& mesh : scene.meshes) {
			import::transform(mesh.obj(), transform);
		}
	}

	import_assets(scene.meshes);
	import_assets(scene.animations);
	import_assets(scene.images);

	context()->ui().refresh_all();
}

}
