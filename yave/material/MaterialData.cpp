/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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


namespace yave {

MaterialData::MaterialData(std::array<AssetPtr<Texture>, texture_count>&& textures) :
        _textures(std::move(textures)) {
}

MaterialData& MaterialData::set_texture(Textures type, AssetPtr<Texture> tex) {
    _textures[usize(type)] = std::move(tex);
    return *this;
}

MaterialData& MaterialData::set_texture_reset_constants(Textures type, AssetPtr<Texture> tex) {
    switch(type) {
        case Roughness:
            _roughness_mul = 1.0f;
        break;

        case Metallic:
            _metallic_mul = 1.0f;
        break;

        case Emissive:
            _emissive_mul = tex.is_empty() ? 0.0f : 1.0f;
        break;

        default:
        break;
    }

    return set_texture(type, std::move(tex));
}

bool MaterialData::is_empty() const {
    return std::all_of(_textures.begin(), _textures.end(), [](const auto& tex) { return tex.is_empty(); });
}

const AssetPtr<Texture>& MaterialData::operator[](Textures tex) const {
    return _textures[usize(tex)];
}

const std::array<AssetPtr<Texture>, MaterialData::texture_count>& MaterialData::textures() const {
    return  _textures;
}

std::array<AssetId, MaterialData::texture_count> MaterialData::texture_ids() const {
    std::array<AssetId, texture_count> ids;
    std::transform(_textures.begin(), _textures.end(), ids.begin(), [](const auto& tex) { return tex.id(); });
    return ids;
}

math::Vec3 MaterialData::emissive_mul() const {
    return _emissive_mul;
}

math::Vec3& MaterialData::emissive_mul() {
    return _emissive_mul;
}

math::Vec3 MaterialData::base_color_mul() const {
    return _base_color_mul;
}

math::Vec3& MaterialData::base_color_mul() {
    return _base_color_mul;
}

float MaterialData::roughness_mul() const {
    return _roughness_mul;
}

float& MaterialData::roughness_mul() {
    return _roughness_mul;
}

float MaterialData::metallic_mul() const {
    return _metallic_mul;
}

float& MaterialData::metallic_mul() {
    return _metallic_mul;
}

bool MaterialData::alpha_tested() const {
    return _alpha_tested;
}

bool& MaterialData::alpha_tested() {
    return _alpha_tested;
}

bool MaterialData::double_sided() const {
    return _double_sided;
}

bool& MaterialData::double_sided() {
    return _double_sided;
}

bool MaterialData::has_emissive() const {
    return !_textures[Textures::Emissive].is_empty() || !_emissive_mul.is_zero();
}

}

