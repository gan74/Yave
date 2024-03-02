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
#include "Material.h"
#include "MaterialTemplate.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/device/MaterialAllocator.h>

#include <y/utils/log.h>

namespace yave {

static DeviceResources::MaterialTemplates material_template_for_data(const MaterialData& data) {
    if(data.material_type() == MaterialData::Type::Specular) {
        if(data.alpha_tested()) {
            return data.double_sided()
                ? DeviceResources::TexturedSpecularAlphaDoubleSidedMaterialTemplate
                : DeviceResources::TexturedSpecularAlphaMaterialTemplate
            ;
        }
        if(data.double_sided()) {
            log_msg("Double sided is not supported for given material configuration", Log::Warning);
        }
        return DeviceResources::TexturedSpecularMaterialTemplate;
    } else {
        if(data.alpha_tested()) {
            return data.double_sided()
                ? DeviceResources::TexturedAlphaDoubleSidedMaterialTemplate
                : DeviceResources::TexturedAlphaMaterialTemplate
            ;
        }
        if(data.double_sided()) {
            log_msg("Double sided is not supported for given material configuration", Log::Warning);
        }
        return DeviceResources::TexturedMaterialTemplate;
    }
}


Material::Material(MaterialData&& data) : Material(device_resources()[material_template_for_data(data)], std::move(data)) {
}

Material::Material(const MaterialTemplate* tmp, MaterialData&& data) :
        _template(tmp),
        _draw_data(material_allocator().allocate_material(data)),
        _data(std::move(data)) {
    y_debug_assert(!is_null());
}

Material::~Material() {
    if(!is_null()) {
        destroy_graphic_resource(std::move(_draw_data));
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
    std::swap(_draw_data, other._draw_data);
    std::swap(_data, other._data);
}

bool Material::is_null() const {
    return _template == nullptr;
}

const MaterialTemplate* Material::material_template() const {
    return _template;
}

const MaterialDrawData& Material::draw_data() const {
    return _draw_data;
}

}
