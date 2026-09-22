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

#include "MaterialEditor.h"
#include "AssetSelector.h"

#include <editor/utils/ui.h>
#include <editor/utils/assets.h>

#include <yave/assets/AssetLoader.h>
#include <yave/graphics/images/Image.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/material/MaterialTemplateData.h>

namespace editor {

static const ImGuiTableFlags table_flags =
    ImGuiTableFlags_BordersInnerV |
    ImGuiTableFlags_BordersInnerH |
    ImGuiTableFlags_Resizable;

static const ImGuiColorEditFlags color_flags =
    ImGuiColorEditFlags_NoSidePreview |
    ImGuiColorEditFlags_NoAlpha |
    ImGuiColorEditFlags_Float |
    ImGuiColorEditFlags_InputRGB;

static void property_label(const char* name) {
    imgui::table_begin_next_row();
    ImGui::TextUnformatted(name);
    ImGui::TableNextColumn();
}

static void edit_texture(AssetPtr<Texture>& texture, const char* name) {
    property_label(name);

    bool clear = false;
    if(imgui::asset_selector(texture.id(), AssetType::Image, asset_type_name(AssetType::Image, false, false), &clear)) {
        add_top_level_widget<AssetSelector>(AssetType::Image)->set_selected_callback(
            [&texture](AssetId id) {
                if(const auto loaded = asset_loader().load_res<Texture>(id)) {
                    texture = loaded.unwrap();
                }
                return true;
            });
    } else if(clear) {
        texture = {};
    }
}

static void edit_color(math::Vec3& color, const char* name) {
    property_label(name);

    if(ImGui::ColorButton("##color", to_im(math::Vec4(color, 1.0f)), color_flags)) {
        ImGui::OpenPopup("##color");
    }
    if(ImGui::BeginPopup("##color")) {
        ImGui::ColorPicker3("##picker", color.begin(), color_flags);
        ImGui::EndPopup();
    }
}

static void edit_float(float& value, const char* name, float min = 0.0f, float max = 1.0f) {
    property_label(name);
    ImGui::DragFloat("##value", &value, 0.01f, min, max);
}

static void edit_bool(bool& value, const char* name) {
    property_label(name);
    ImGui::Checkbox("##value", &value);
}

static void edit_blend_mode(BlendMode& mode) {
    property_label("Blend");

    const char* names[] = {"None", "Add", "SrcAlpha"};
    int current = int(mode);
    if(ImGui::Combo("##blend", &current, names, int(std::size(names)))) {
        mode = BlendMode(current);
    }
}

static void edit_common(MaterialData::CommonMaterialData& common) {
    edit_texture(common.diffuse, "Diffuse");
    edit_texture(common.normal, "Normal");
    edit_texture(common.emissive, "Emissive");
    edit_color(common.color_factor, "Color");
    edit_color(common.emissive_factor, "Emissive factor");
    edit_blend_mode(common.blend_mode);
    edit_bool(common.alpha_tested, "Alpha tested");
    edit_bool(common.double_sided, "Double sided");
}

MaterialEditor::MaterialEditor(MaterialWorkspace* ws) :
        WorkspaceWidget(ICON_FA_BRUSH " Material Editor", ws) {
}

void MaterialEditor::on_gui() {
    auto& material = workspace()->material();

    const bool is_metallic = std::holds_alternative<MaterialData::MetallicRoughnessMaterialData>(material);
    int type = is_metallic ? 0 : 1;
    const char* type_names[] = {"MetallicRoughness", "Specular"};
    if(ImGui::Combo("Type", &type, type_names, int(std::size(type_names)))) {
        MaterialData::CommonMaterialData common = std::visit([](auto& data) { return data.common; }, material);
        if(type == 0) {
            MaterialData::MetallicRoughnessMaterialData metallic;
            metallic.common = std::move(common);
            material = std::move(metallic);
        } else {
            MaterialData::SpecularMaterialData specular;
            specular.common = std::move(common);
            material = std::move(specular);
        }
    }

    ImGui::Separator();

    if(!ImGui::BeginTable("##properties", 2, table_flags)) {
        return;
    }

    ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

    std::visit(Overloaded{
        [](MaterialData::MetallicRoughnessMaterialData& data) {
            edit_common(data.common);
            edit_texture(data.metallic_roughness, "Metallic/Roughness");
            edit_float(data.metallic_factor, "Metallic");
            edit_float(data.roughness_factor, "Roughness");
        },
        [](MaterialData::SpecularMaterialData& data) {
            edit_common(data.common);
            edit_texture(data.specular, "Specular");
            edit_texture(data.specular_color, "Specular color");
            edit_color(data.specular_color_factor, "Specular color factor");
            edit_float(data.specular_factor, "Specular");
        },
    }, material);

    ImGui::EndTable();
}

}
