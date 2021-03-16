/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include "MaterialEditor.h"
#include "AssetSelector.h"

#include <editor/utils/ui.h>
#include <editor/EditorApplication.h>

#include <yave/graphics/images/ImageData.h>
#include <yave/assets/AssetLoader.h>
#include <yave/utils/color.h>

#include <y/io2/Buffer.h>

#include <y/utils/log.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

Y_TODO(move into utils)
static bool color_picker(const char* name, math::Vec3* color) {
    const int flags =
        ImGuiColorEditFlags_NoSidePreview |
        ImGuiColorEditFlags_NoAlpha |
        ImGuiColorEditFlags_Float |
        ImGuiColorEditFlags_InputRGB;

    if(ImGui::ColorButton(name, math::Vec4(*color, 1.0f), flags)) {
        ImGui::OpenPopup("Color##popup");
    }

    bool changed = false;
    if(ImGui::BeginPopup("Color##popup")) {
        changed = ImGui::ColorPicker3("##colorpicker", color->begin(), flags);
        ImGui::EndPopup();
    }

    if(changed && color->length2() < 0.001f) {
        *color = math::Vec3();
    }

    return changed;
}

static void save(SimpleMaterialData data, AssetPtr<Material>& material) {
    io2::Buffer buffer;
    serde3::WritableArchive arc(buffer);
    if(!arc.serialize(data)) {
        log_msg("Unable to serialize material", Log::Error);
        return;
    }

    buffer.reset();
    if(!asset_store().write(material.id(), buffer)) {
        log_msg("Unable to write material", Log::Error);
        return;
    }

    {
        y_profile_zone("reload");
        material.reload();
        application()->flush_reload();
    }
}

static void set_tex_and_save(AssetPtr<Material>& material, SimpleMaterialData::Textures index, AssetId id) {
    y_profile();
    const auto tex = asset_loader().load_res<Texture>(id);
    if(!tex || tex.unwrap().is_failed()) {
        log_msg("Unable to load texture", Log::Error);
        return;
    }

    SimpleMaterialData data = material->data();
    data.set_texture_reset_constants(index, std::move(tex.unwrap()));

    save(std::move(data), material);
}



MaterialEditor::MaterialEditor() :  Widget(ICON_FA_BRUSH " Material Editor") {
}

void MaterialEditor::set_material(const AssetPtr<Material>& mat) {
    _material = mat;
}

void MaterialEditor::on_gui() {
    y_profile();

    if(_material) {
        _preview.set_material(_material);
        _preview.draw_gui_inside();
    }

    if(imgui::asset_selector(_material.id(), AssetType::Material, "Material")) {
        add_child_widget<AssetSelector>(AssetType::Material)->set_selected_callback(
            [this](AssetId asset) {
                if(const auto mat = asset_loader().load_res<Material>(asset)) {
                    set_material(mat.unwrap());
                }
                return true;
            });
    }

    Y_TODO(If the material fails to load we cant ever get it back)
    if(!_material) {
        return;
    }

    SimpleMaterialData data = _material->data();

    const std::array<const char*, SimpleMaterialData::texture_count> texture_names = {"Diffuse", "Normal", "Roughness", "Metallic", "Emissive"};
    auto texture_selector = [&](SimpleMaterialData::Textures tex) {
        bool clear = false;
        if(imgui::asset_selector(data.textures()[tex].id(), AssetType::Image, texture_names[tex], &clear)) {
            add_child_widget<AssetSelector>(AssetType::Image)->set_selected_callback(
                [=](AssetId id) {
                    set_tex_and_save(_material, tex, id);
                    return true;
            });
        }
        if(clear) {
            set_tex_and_save(_material, tex, AssetId::invalid_id());
        }
    };

    if(ImGui::CollapsingHeader(texture_names[SimpleMaterialData::Diffuse])) {
        texture_selector(SimpleMaterialData::Diffuse);
    }

    if(ImGui::CollapsingHeader(texture_names[SimpleMaterialData::Normal])) {
        texture_selector(SimpleMaterialData::Normal);
    }

    if(ImGui::CollapsingHeader(texture_names[SimpleMaterialData::Roughness])) {
        texture_selector(SimpleMaterialData::Roughness);
        if(data.textures()[SimpleMaterialData::Roughness].is_empty()) {
            float& roughness = data.constants().roughness_mul;
            if(ImGui::SliderFloat("Roughness##slider", &roughness, 0.0f, 1.0f)) {
                save(data, _material);
            }
        }
    }

    if(ImGui::CollapsingHeader(texture_names[SimpleMaterialData::Metallic])) {
        texture_selector(SimpleMaterialData::Metallic);
        if(data.textures()[SimpleMaterialData::Metallic].is_empty()) {
            bool metallic = data.constants().metallic_mul > 0.5f;
            if(ImGui::Checkbox("Metallic##box", &metallic)) {
                data.constants().metallic_mul = metallic ? 1.0f : 0.0f;
                save(data, _material);
            }
        }
    }

    if(ImGui::CollapsingHeader(texture_names[SimpleMaterialData::Emissive])) {
        texture_selector(SimpleMaterialData::Emissive);
        if(data.textures()[SimpleMaterialData::Emissive].is_empty()) {
            ImGui::Text("Emissive");
            ImGui::SameLine();
            math::Vec3& color = data.constants().emissive_mul;
            if(color_picker("Emissive##picker", &color)) {
                save(data, _material);
            }
        }
    }

    if(ImGui::Checkbox("Alpha tested", &data.alpha_tested())) {
        save(data, _material);
    }
}

}

