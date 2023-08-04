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
#include "Material.h"
#include "MaterialTemplate.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/images/TextureLibrary.h>

namespace yave {

static auto material_texture_views(const MaterialData& data) {
    std::array<TextureView, MaterialData::texture_count> textures = {
        *device_resources()[DeviceResources::GreyTexture],          // Diffuse
        *device_resources()[DeviceResources::FlatNormalTexture],    // Normal
        *device_resources()[DeviceResources::WhiteTexture],         // Roughness
        *device_resources()[DeviceResources::WhiteTexture],         // Metallic
        *device_resources()[DeviceResources::WhiteTexture],         // Emissive
    };

    for(usize i = 0; i != MaterialData::texture_count; ++i) {
        y_debug_assert(!data.textures()[i].is_loading());
        if(const auto* tex = data.textures()[i].get()) {
            textures[i] = *tex;
        }
    }

    return textures;
}

static DeviceResources::MaterialTemplates material_template_for_data(const MaterialData& data) {
    if(data.alpha_tested()) {
       return DeviceResources::TexturedAlphaMaterialTemplate;
    }
    return DeviceResources::TexturedMaterialTemplate;
}



Material::Material(MaterialData&& data) : Material(device_resources()[material_template_for_data(data)], std::move(data)) {
}

Material::Material(const MaterialTemplate* tmp, MaterialData&& data) : _template(tmp), _data(std::move(data)) {
    if(!is_null()) {
        const auto textures = material_texture_views(_data);
        _shader_data.constants = _data.constants();
        for(usize i = 0; i != textures.size(); ++i) {
            _shader_data.textures_indices[i] = texture_library().add_texture(textures[i]);
        }
    }
}

Material::~Material() {
    if(!is_null()) {
        // Todo, textures might still be used by command buffer
        for(const TextureView& tex : material_texture_views(_data)) {
            texture_library().remove_texture(tex);
        }
    }
}

Material::Material(Material&& other) {
    swap(other);
}

Material& Material::operator=(Material&& other) {
    swap(other);
    return *this;
}

void Material::swap(Material& other) {
    std::swap(_template, other._template);
    std::swap(_shader_data, other._shader_data);
    std::swap(_data, other._data);
}

const MaterialData& Material::data() const {
    return _data;
}

const MaterialTemplate* Material::material_template() const {
    return _template;
}

const Material::ShaderData& Material::shader_data() const {
    return _shader_data;
}

bool Material::is_null() const {
    return _template == nullptr;
}

}

