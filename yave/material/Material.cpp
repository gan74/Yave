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
#include "Material.h"
#include "MaterialTemplate.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/device/MaterialAllocator.h>

#include <y/utils/log.h>

namespace yave {

Material::Material(MaterialData&& mat_data) :
    _draw_data(material_allocator().allocate_material(mat_data)),
    _data(std::move(mat_data)) {

    y_debug_assert(!is_null());

    if(_data.material_type() == MaterialData::Type::Specular) {
        if(_data.alpha_tested()) {
            if(_data.double_sided()) {
                _templates[usize(PassType::GBuffer)] = device_resources()[DeviceResources::GBufferTexturedSpecularAlphaDoubleSidedMaterialTemplate];
                _templates[usize(PassType::Forward)] = device_resources()[DeviceResources::ForwardTexturedSpecularAlphaDoubleSidedMaterialTemplate];
            } else {
                _templates[usize(PassType::GBuffer)] = device_resources()[DeviceResources::GBufferTexturedSpecularAlphaMaterialTemplate];
                _templates[usize(PassType::Forward)] = device_resources()[DeviceResources::ForwardTexturedSpecularAlphaMaterialTemplate];
            }
        } else {
            if(_data.double_sided()) {
                y_only_once(log_msg("Double sided is not supported for given material configuration", Log::Warning));
            }
            _templates[usize(PassType::GBuffer)] = device_resources()[DeviceResources::GBufferTexturedSpecularMaterialTemplate];
            _templates[usize(PassType::Forward)] = device_resources()[DeviceResources::ForwardTexturedSpecularMaterialTemplate];
        }
    } else {
        if(_data.alpha_tested()) {
            if(_data.double_sided()) {
                _templates[usize(PassType::GBuffer)] = device_resources()[DeviceResources::GBufferTexturedAlphaDoubleSidedMaterialTemplate];
                _templates[usize(PassType::Forward)] = device_resources()[DeviceResources::ForwardTexturedAlphaDoubleSidedMaterialTemplate];
            } else {
                _templates[usize(PassType::GBuffer)] = device_resources()[DeviceResources::GBufferTexturedAlphaMaterialTemplate];
                _templates[usize(PassType::Forward)] = device_resources()[DeviceResources::ForwardTexturedAlphaMaterialTemplate];
            }
        } else {
            if(_data.double_sided()) {
                y_only_once(log_msg("Double sided is not supported for given material configuration", Log::Warning));
            }
            _templates[usize(PassType::GBuffer)] = device_resources()[DeviceResources::GBufferTexturedMaterialTemplate];
            _templates[usize(PassType::Forward)] = device_resources()[DeviceResources::ForwardTexturedMaterialTemplate];
        }
    }

    for(const MaterialTemplate* tmp : _templates) {
        if(tmp && tmp->data().depth_write()) {
            _templates[usize(PassType::Depth)] = tmp;
            break;
        }
    }

    // We do not need this
    // _templates[usize(PassType::Id)] = device_resources()[DeviceResources::IdMaterialTemplate];
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
    std::swap(_templates, other._templates);
    std::swap(_draw_data, other._draw_data);
    std::swap(_data, other._data);
}

bool Material::is_null() const {
    return _draw_data.is_null();
}

bool Material::is_transparent() const {
    return _data.is_transparent();
}

bool Material::alpha_tested() const {
    return _data.alpha_tested();
}

const MaterialTemplate* Material::material_template(PassType pass_type) const {
    return _templates[usize(pass_type)];
}

const MaterialDrawData& Material::draw_data() const {
    return _draw_data;
}

}
