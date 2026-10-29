/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

#include "GltfImporter.h"

#include <editor/import/import.h>

#include <yave/assets/AssetStore.h>
#include <yave/utils/FileSystemModel.h>

#include <editor/utils/ui.h>

#include <y/core/Chrono.h>


namespace editor {

GltfImporter::GltfImporter() : GltfImporter(asset_store().filesystem()->current_path().unwrap_or(".")) {
}

GltfImporter::GltfImporter(core::String dst_import_path) :
        Widget("glTF importer"),
        _job_system(4_uu) {

    _settings.import_path = std::move(dst_import_path);
    _browser.set_selection_filter(import::supported_scene_extensions());
    _browser.set_canceled_callback([this] { close(); return true; });
    _browser.set_selected_callback([this](const auto& filename) {
        _job_system.schedule([=, this] {
            y_profile_zone("parsing import");
            core::DebugTimer timer("Parsing gltf");
            _scene = import::gltf::load_gltf(filename);
        });
        y_debug_assert(_state == State::Browsing);
        _state = State::Parsing;
        return true;
    });
}

bool GltfImporter::should_keep_alive() const {
     return !_job_system.is_empty();
}

void GltfImporter::cancel() {
    if(_import_status) {
        _import_status->cancelled = true;
    }
    close();
}

void GltfImporter::on_gui() {
    switch(_state) {
        case State::Browsing:
            _browser.draw_gui_inside();
        break;

        case State::Parsing:
            ImGui::Text("Parsing scene%s", imgui::ellipsis());
            if(_job_system.is_empty()) {
                _state = _scene.is_error() ? State::Failed : State::Settings;
            }

            if(ImGui::Button("Cancel")) {
                cancel();
            }
        break;

        case State::Settings: {
            y_debug_assert(_scene.is_ok());

            const import::gltf::Scene& scene = _scene.unwrap();
            ImGui::Text("%u meshes, %u materials, %u images, %u nodes", u32(scene.meshes.size()), u32(scene.materials.size()), u32(scene.images.size()), u32(scene.nodes.size()));

            if(imgui::path_selector("Import path:", _settings.import_path)) {
                FileBrowser* browser = add_top_level_widget<FileBrowser>(asset_store().filesystem());
                browser->set_selection_filter("", FileBrowser::FilterFlags::IncludeDirs);
                browser->set_selected_callback([this](const auto& filename) {
                    if(asset_store().filesystem()->is_directory(filename).unwrap_or(false)) {
                        _settings.import_path = filename;
                        return true;
                    }
                    return false;
                });
            }

            ImGui::Checkbox("Import children prefabs as assets", &_settings.import_child_prefabs_as_assets);
            ImGui::Checkbox("Create colliders", &_settings.create_colliders);
            ImGui::Checkbox("Compress textures", &_settings.compress_textures);

            if(ImGui::Button(ICON_FA_CHECK " Import")) {
                std::tie(_import_job, _import_status) = import::schedule_gltf_import(_job_system, std::move(_scene.unwrap()), _settings);
                _scene = core::Err();
                _state = State::Importing;
            }
        } break;

        case State::Importing: {
            y_debug_assert(_import_status);

            ImGui::Text("Importing%s (%u/%u)", imgui::ellipsis(), u32(_import_status->done), u32(_import_status->total));
            if(_import_job.is_finished()) {
                refresh_all();
                _state = State::Done;
            } else if(ImGui::Button("Cancel")) {
                cancel();
            }
        } break;

        case State::Done: {
            if(const usize failed = _import_status->failed) {
                ImGui::Text("Done, %u out of %u assets failed to import, check the log for details", u32(failed), u32(_import_status->total));
            } else {
                ImGui::TextUnformatted("Done!");
            }

            if(ImGui::Button("Ok")) {
                close();
            }
        } break;


        case State::Failed: {
            ImGui::TextUnformatted("Failed!");

            if(ImGui::Button("Ok")) {
                close();
            }
        } break;
    }
}

}

