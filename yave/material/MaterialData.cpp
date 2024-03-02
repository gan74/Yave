/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include "MaterialData.h"

#include <yave/graphics/shader_structs.h>

namespace yave {

MaterialData::MaterialData(MetallicRoughnessMaterialData data) : MaterialData(Type::MetallicRoughness, std::move(data.common)) {
    _textures[shader::metallic_roughness_texture_index] = std::move(data.metallic_roughness);

    _metallic_factor = data.metallic_factor;
    _roughness_factor = data.roughness_factor;
}

MaterialData::MaterialData(SpecularMaterialData data) : MaterialData(Type::Specular, std::move(data.common)) {
    _textures[shader::specular_texture_index] = std::move(data.specular);
    _textures[shader::specular_color_texture_index] = std::move(data.specular_color);

    _specular_color_factor = data.specular_color_factor;
    _specular_factor = data.specular_factor;
}

MaterialData::MaterialData(Type type, CommonMaterialData data) : _type(type) {
    _textures[shader::diffuse_texture_index] = std::move(data.diffuse);
    _textures[shader::normal_texture_index] = std::move(data.normal);
    _textures[shader::emissive_texture_index] = std::move(data.emissive);

    _base_color_factor = data.color_factor;
    _emissive_factor = data.emissive_factor;
    _alpha_tested = data.alpha_tested;
    _double_sided = data.double_sided;
}


MaterialData::Type MaterialData::material_type() const {
    return _type;
}

bool MaterialData::is_empty() const {
    return std::all_of(_textures.begin(), _textures.end(), [](const auto& tex) { return tex.is_empty(); });
}

core::Span<AssetPtr<Texture>> MaterialData::textures() const {
    return _textures;
}

std::array<AssetId, MaterialData::max_texture_count> MaterialData::texture_ids() const {
    std::array<AssetId, max_texture_count> ids;
    std::transform(_textures.begin(), _textures.end(), ids.begin(), [](const auto& tex) { return tex.id(); });
    return ids;
}

math::Vec3 MaterialData::emissive_factor() const {
    return _emissive_factor;
}

math::Vec3 MaterialData::base_color_factor() const {
    return _base_color_factor;
}

math::Vec3 MaterialData::specular_color() const {
    return _specular_color_factor;
}

float MaterialData::roughness_factor() const {
    return _roughness_factor;
}

float MaterialData::metallic_factor() const {
    return _metallic_factor;
}

float MaterialData::specular_factor() const {
    return _specular_factor;
}

bool MaterialData::alpha_tested() const {
    return _alpha_tested;
}

bool MaterialData::double_sided() const {
    return _double_sided;
}

bool MaterialData::has_emissive() const {
    return !_textures[shader::emissive_texture_index].is_empty() || !_emissive_factor.is_zero();
}

}

