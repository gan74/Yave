/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#include "SceneImporter2.h"
#include "Renamer.h"

#include <editor/utils/ui.h>

#include <yave/assets/AssetLoader.h>
#include <yave/utils/FileSystemModel.h>

#include <y/io2/Buffer.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

template<typename T>
static usize count_errors(const core::Vector<T>& assets) {
    usize errors = 0;
    for(const T& asset : assets) {
        errors += asset.is_error;
    }
    return errors;
}

template<typename T>
static void set_global_import_flag(core::Vector<T>& assets) {
    ImGui::PushID(&assets);

    bool import = false;
    for(const T& asset : assets) {
        if(asset.import && !asset.is_error) {
            import = true;
            break;
        }
    }

    if(ImGui::Checkbox("##import", &import)) {
        for(T& asset : assets) {
            if(!asset.is_error) {
                asset.import = import;
            }
        }
    }

    ImGui::PopID();
}


static const char* make_display_name(const import::ParsedScene::Asset& asset) {
    return asset.is_error
        ? fmt_c_str(ICON_FA_EXCLAMATION_TRIANGLE " %", asset.name)
        : asset.name.data();
}

static bool patch_import_error(import::ParsedScene::Asset& asset) {
    asset.import &= !asset.is_error;
    if(asset.is_error) {
        ImGui::PushStyleColor(ImGuiCol_Text, math::Vec4(1.0f, 0.3f, 0.3f, 1.0f));
    }
    return asset.is_error;
}

template<typename T>
static void display_asset(T& asset) {
    ImGui::PushID(&asset);
    {
        const bool is_error = patch_import_error(asset);

        ImGui::Checkbox("##import", &asset.import);
        ImGui::SameLine();
        ImGui::Selectable(make_display_name(asset));

        if(!is_error) {
            if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                add_child_widget<Renamer>(asset.name, [&](std::string_view new_name) { asset.name = new_name; return true; });
            }
        } else {
            ImGui::PopStyleColor();
            if(ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Asset \"%s\" could not be parsed and won't be imported", asset.name.data());
            }
        }
    }
    ImGui::PopID();
}

static void display_mesh_import_list(import::ParsedScene& scene) {
    set_global_import_flag(scene.meshes);
    ImGui::SameLine();

    if(ImGui::TreeNode(fmt_c_str("% Meshes", scene.meshes.size()))) {
        ImGui::Indent();

        for(auto& mesh : scene.meshes) {
            display_asset(mesh);
        }

        ImGui::Unindent();
        ImGui::TreePop();
    }
}

static void display_material_import_list(import::ParsedScene& scene) {
    set_global_import_flag(scene.materials);
    ImGui::SameLine();

    if(ImGui::TreeNode(fmt_c_str("% Materials", scene.materials.size()))) {
        ImGui::Indent();

        for(auto& material : scene.materials) {
            display_asset(material);
        }

        ImGui::Unindent();
        ImGui::TreePop();
    }
}

static void display_image_import_list(import::ParsedScene& scene) {
    set_global_import_flag(scene.images);
    ImGui::SameLine();

    if(ImGui::TreeNode(fmt_c_str("% Textures", scene.images.size()))) {
        ImGui::Indent();

        for(auto& image : scene.images) {
            display_asset(image);
        }

        ImGui::Unindent();
        ImGui::TreePop();
    }
}



SceneImporter2::SceneImporter2() : SceneImporter2(asset_store().filesystem()->current_path().unwrap_or(".")) {
}

SceneImporter2::SceneImporter2(std::string_view import_dst_path) :
        Widget("Scene importer"),
        _import_path(import_dst_path) {

    _browser.set_selection_filter(false, import::supported_scene_extensions());
    _browser.set_canceled_callback([this] { close(); return true; });
    _browser.set_selected_callback([this](const auto& filename) {
        _future = std::async(std::launch::async, [=] {
            _parsed_scene = import::parse_scene(filename);
        });
        y_debug_assert(_state == State::Browsing);
        _state = State::Parsing;
        return true;
    });
}

template<typename T>
static void import_single_asset(const T& asset, std::string_view name, AssetType type) {
    y_profile_zone("asset import");

    io2::Buffer buffer;
    serde3::WritableArchive arc(buffer);
    if(auto sr = arc.serialize(asset); sr.is_error()) {
        log_msg(fmt("Unable serialize \"%\": error %", name, sr.error().type), Log::Error);
    } else {
        buffer.reset();

        usize emergency_id = 1;
        core::String import_name = name;

        for(;;) {
            const auto result = asset_store().import(buffer, import_name, type);
            if(result.is_error()) {
                if(result.error() == AssetStore::ErrorType::NameAlreadyExists) {
                    import_name = fmt("%_(%)", name, emergency_id++);
                    continue;
                }
                log_msg(fmt("Unable to import \"%\": error %", import_name, result.error()), Log::Error);
            } else {
                log_msg(fmt("Saved asset as \"%\"", import_name));
            }

            break;
        }

    }
}

void SceneImporter2::on_gui() {
    switch(_state) {
        case State::Browsing:
            _browser.draw_gui_inside();
        break;

        case State::Parsing:
            ImGui::Text("Parsing scene...");
            if(_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                _future.get();
                if(_parsed_scene.is_error) {
                    _state = State::Browsing;
                } else {
                    _state = State::Settings;
                }
            }
        break;

        case State::Settings: {
            const usize total_errors =
                count_errors(_parsed_scene.meshes) +
                count_errors(_parsed_scene.materials) +
                count_errors(_parsed_scene.images);

            if(_parsed_scene.is_error) {
                ImGui::TextColored(math::Vec4(1.0f, 0.3f, 0.3f, 1.0f), ICON_FA_EXCLAMATION_TRIANGLE " Error while parsing scene, some elements might be missing or broken");
                ImGui::Separator();
            } else if(total_errors) {
                ImGui::TextColored(math::Vec4(1.0f, 0.3f, 0.3f, 1.0f), ICON_FA_EXCLAMATION_TRIANGLE " %u assets could not be parsed and won't be imported", u32(total_errors));
                ImGui::Separator();
            }

            ImGui::Checkbox("Import as scene", &_parsed_scene.import_scene);

            ImGui::Separator();

            const float button_height = ImGui::GetFont()->FontSize + ImGui::GetStyle().FramePadding.y * 2.0f + 4.0f;
            if(ImGui::BeginChild("##assets", ImVec2(0, ImGui::GetContentRegionAvail().y - button_height), true)) {
                display_mesh_import_list(_parsed_scene);
                display_material_import_list(_parsed_scene);
                display_image_import_list(_parsed_scene);
            }
            ImGui::EndChild();

            if(ImGui::Button(ICON_FA_CHECK " Import")) {
                _future = std::async(std::launch::async, [this] {

                    {
                        const core::String image_import_path = asset_store().filesystem()->join(_import_path, "Textures");
                        for(usize i = 0; i != _parsed_scene.images.size(); ++i) {
                            const auto& image = _parsed_scene.images[i];
                            if(image.import) {
                                if(auto res = _parsed_scene.build_image_data(i)) {
                                    import_single_asset(res.unwrap(), asset_store().filesystem()->join(image_import_path, image.name), AssetType::Image);
                                }
                            }
                        }
                    }

                    {
                        const core::String mesh_import_path = asset_store().filesystem()->join(_import_path, "Meshes");
                        for(usize i = 0; i != _parsed_scene.meshes.size(); ++i) {
                            const auto& mesh = _parsed_scene.meshes[i];
                            if(mesh.import) {
                                core::Vector<core::Result<MeshData>> sub_meshes = _parsed_scene.build_mesh_data(i);
                                for(usize j = 0; j != sub_meshes.size(); ++j) {
                                    if(auto& res = sub_meshes[j]) {
                                        import_single_asset(std::move(res.unwrap()), asset_store().filesystem()->join(mesh_import_path, mesh.name + "_" + mesh.sub_meshes[j].name), AssetType::Mesh);
                                    }
                                }
                            }
                        }
                    }
                });
                _state = State::Importing;
            }
        } break;

        case State::Importing: {
            ImGui::Text("Importing assets...");
            if(_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                _future.get();
                close();
            }
        } break;
    }
}

}

