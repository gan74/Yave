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

#include "MaterialWorkspace.h"

#include "editor.h"
#include "UiManager.h"

#include <yave/assets/AssetLoader.h>
#include <yave/assets/AssetStore.h>
#include <yave/graphics/images/Image.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/graphics/shader_structs.h>
#include <yave/material/Material.h>

#include <y/io2/Buffer.h>
#include <y/serde3/archives.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

namespace editor {

static MaterialData::CommonMaterialData common_from_material(const MaterialData& data) {
    const auto textures = data.textures();

    MaterialData::CommonMaterialData common;
    common.diffuse = textures[shader::TextureSlots::Albedo];
    common.normal = textures[shader::TextureSlots::Normal];
    common.emissive = textures[shader::TextureSlots::Emissive];
    common.color_factor = data.base_color_factor();
    common.emissive_factor = data.emissive_factor();
    common.blend_mode = data.blend_mode();
    common.alpha_tested = data.alpha_tested();
    common.double_sided = data.double_sided();
    return common;
}

static MaterialWorkspace::EditMaterial to_edit_data(const MaterialData& data) {
    if(data.material_type() == MaterialData::Type::Specular) {
        MaterialData::SpecularMaterialData specular;
        specular.common = common_from_material(data);
        specular.specular = data.textures()[shader::TextureSlots::MetallicRoughnessSpecular];
        specular.specular_color = data.textures()[shader::TextureSlots::SpecularColor];
        specular.specular_color_factor = data.specular_color();
        specular.specular_factor = data.specular_factor();
        return specular;
    }

    MaterialData::MetallicRoughnessMaterialData metallic;
    metallic.common = common_from_material(data);
    metallic.metallic_roughness = data.textures()[shader::TextureSlots::MetallicRoughnessSpecular];
    metallic.roughness_factor = data.roughness_factor();
    metallic.metallic_factor = data.metallic_factor();
    return metallic;
}

static void load_texture(AssetPtr<Texture>& texture) {
    if(texture.is_empty()) {
        return;
    }
    if(const auto loaded = asset_loader().load_res<Texture>(texture.id())) {
        texture = loaded.unwrap();
    }
}

static void load_common_textures(MaterialData::CommonMaterialData& common) {
    load_texture(common.diffuse);
    load_texture(common.normal);
    load_texture(common.emissive);
}





MaterialWorkspace::MaterialWorkspace(AssetId id) : _id(id) {
    update_name();
    if(_id != AssetId::invalid_id()) {
        load();
    }
}

MaterialWorkspace::~MaterialWorkspace() {
}

std::string_view MaterialWorkspace::name() const {
    return _name;
}

void MaterialWorkspace::update() {
}

void MaterialWorkspace::save() {
    y_profile();

    if(_id == AssetId::invalid_id()) {
        log_msg("Unable to save material: no asset id", Log::Error);
        return;
    }

    const MaterialData data = material_data();

    io2::Buffer buffer;
    {
        serde3::WritableArchive arc(buffer);
        if(const auto res = arc.serialize(data); res.is_error()) {
            log_msg("Unable to serialize material", Log::Error);
            return;
        }
        buffer.reset();
    }

    const auto textures = data.textures();
    core::Vector<AssetId> refs;
    refs.set_min_capacity(textures.size());
    for(const auto& tex : textures) {
        if(tex.id() != AssetId::invalid_id()) {
            refs << tex.id();
        }
    }

    if(const auto res = asset_store().write(_id, buffer, refs); res.is_error()) {
        log_msg(fmt("Unable to write material, error: {}", res.error()), Log::Error);
        return;
    }

    asset_loader().reload<Material>(_id);
    for(const auto& workspace : ui().workspaces()) {
        workspace->grab_reloaded();
    }

    log_msg("Material saved");
}

void MaterialWorkspace::load() {
    y_profile();

    if(_id == AssetId::invalid_id()) {
        log_msg("Unable to load material: no asset id", Log::Error);
        return;
    }

    const auto reader = asset_store().data(_id);
    if(!reader) {
        log_msg("Unable to find material asset", Log::Error);
        return;
    }

    MaterialData data;
    serde3::ReadableArchive arc(*reader.unwrap());
    if(const auto res = arc.deserialize(data); res.is_error()) {
        log_msg("Unable to load material", Log::Error);
        return;
    } else if(res.unwrap() == serde3::Success::Partial) {
        log_msg("Material was only partially loaded", Log::Warning);
    }

    _material = to_edit_data(data);
    load_textures();
    update_name();

    log_msg("Material loaded");
}

AssetId MaterialWorkspace::asset_id() const {
    return _id;
}

MaterialWorkspace::EditMaterial& MaterialWorkspace::material() {
    return _material;
}

const MaterialWorkspace::EditMaterial& MaterialWorkspace::material() const {
    return _material;
}

MaterialData MaterialWorkspace::material_data() const {
    return std::visit([](const auto& data) { return MaterialData(data); }, _material);
}

void MaterialWorkspace::update_name() {
    _name = "Material";
    if(_id != AssetId::invalid_id()) {
        if(auto name = asset_store().name(_id)) {
            _name = std::move(name.unwrap());
        }
    }
}

void MaterialWorkspace::load_textures() {
    std::visit(Overloaded{
        [](MaterialData::MetallicRoughnessMaterialData& data) {
            load_common_textures(data.common);
            load_texture(data.metallic_roughness);
        },
        [](MaterialData::SpecularMaterialData& data) {
            load_common_textures(data.common);
            load_texture(data.specular);
            load_texture(data.specular_color);
        },
    }, _material);
}

}
