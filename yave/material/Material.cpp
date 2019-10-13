/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#include "SimpleMaterialData.h"

#include <yave/device/Device.h>

namespace yave {

static DescriptorSet create_descriptor_set(DevicePtr dptr, const SimpleMaterialData& data) {
	if(!dptr || data.is_empty()) {
		return DescriptorSet();
	}

	std::array<Binding, SimpleMaterialData::texture_count> bindings = {
			*dptr->device_resources()[DeviceResources::WhiteTexture],
			*dptr->device_resources()[DeviceResources::FlatNormalTexture],
			*dptr->device_resources()[DeviceResources::RedTexture]
		};
	for(usize i = 0; i != SimpleMaterialData::texture_count; ++i) {
		if(const auto& tex = data.textures()[i]) {
			bindings[i] = *tex;
		}
	}
	return DescriptorSet(dptr, bindings);
}


Material::Material(DevicePtr dptr, SimpleMaterialData&& data) :
		_template(dptr->device_resources()[data.is_empty() ? DeviceResources::BasicMaterialTemplate : DeviceResources::TexturedMaterialTemplate]),
		_set(create_descriptor_set(device(), data)),
		_data(std::move(data)) {
}

Material::Material(const MaterialTemplate* tmp, SimpleMaterialData&& data) :
		_template(tmp),
		_set(create_descriptor_set(device(), data)),
		_data(std::move(data)) {
}

const SimpleMaterialData& Material::data() const {
	return _data;
}

const DescriptorSetBase& Material::descriptor_set() const {
	return _set;
}

const MaterialTemplate* Material::mat_template() const {
	return _template;
}

DevicePtr Material::device() const {
	return _template->device();
}

bool Material::is_null() const {
	return !_template;
}

}
